#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/etherdevice.h>
#include <linux/of_net.h>
#include <linux/of_platform.h>
#include <linux/phylink.h>
#include <linux/ethtool.h>
#include <linux/if.h>
#include <linux/if_vlan.h>
#include <linux/crc32.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/irq.h>

#include "dma.h"
#include "eth.h"
#include "sfxgmac-ext.h"

#ifdef FPGA
#include <linux/of_mdio.h>
#endif

static const char * const gmac_gstring_stats[] = {
	"tx_bytes",
	"tx_bytes_good",
	"tx_packets",
	"tx_packets_good",
	"tx_unicast_packets_good",
	"tx_broadcast_packets_good",
	"tx_multicast_packets_good",
	"tx_64_byte_packets",
	"tx_65_to_127_byte_packets",
	"tx_128_to_255_byte_packets",
	"tx_256_to_511_byte_packets",
	"tx_512_to_1023_byte_packets",
	"tx_1024_to_max_byte_packets",
	"tx_underflow_errors",
	"tx_late_collision_errors",
	"tx_excessive_collision_errors",
	"tx_carrier_sense_errors",
	"tx_excessive_deferral_errors",
	"tx_pause_frames",
	"tx_vlan_packets_good",
	"tx_oversize_packets_good",
	"rx_bytes",
	"rx_bytes_good",
	"rx_packets",
	"rx_unicast_packets_good",
	"rx_broadcast_packets_good",
	"rx_multicast_packets_good",
	"rx_crc_errors",
	"rx_alignment_errors",
	"rx_crc_errors_small_packets",
	"rx_crc_errors_giant_packets",
	"rx_undersize_packets_good",
	"rx_oversize_packets_good",
	"rx_64_byte_packets",
	"rx_65_to_127_byte_packets",
	"rx_128_to_255_byte_packets",
	"rx_256_to_511_byte_packets",
	"rx_512_to_1023_byte_packets",
	"rx_1024_to_max_byte_packets",
	"rx_length_errors",
	"rx_out_of_range_errors",
	"rx_pause_frames",
	"rx_fifo_overflow_errors",
	"rx_watchdog_errors",
	"rx_frame_extension_errors",
	"rx_control_frames",
	"rx_vlan_packets_good",
};

struct gmac_stats {
	/* MMC TX counters */
	u64 txoctetcount_gb;
	u64 txoctetcount_g;
	u64 txframecount_gb;
	u64 txframecount_g;
	u64 txunicastframes_g;
	u64 txbroadcastframes_g;
	u64 txmulticastframes_g;
	u64 tx64octets_gb;
	u64 tx65to127octets_gb;
	u64 tx128to255octets_gb;
	u64 tx256to511octets_gb;
	u64 tx512to1023octets_gb;
	u64 tx1024tomaxoctets_gb;
	u64 txunderflowerror;
	u64 txlatecol;
	u64 txexcesscol;
	u64 txcarriererror;
	u64 txexcessdef;
	u64 txpauseframes;
	u64 txvlanframes_g;
	u64 txoversize_g;

	/* MMC RX counters */
	u64 rxoctetcount_gb;
	u64 rxoctetcount_g;
	u64 rxframecount_gb;
	u64 rxunicastframes_g;
	u64 rxbroadcastframes_g;
	u64 rxmulticastframes_g;
	u64 rxcrcerror;
	u64 rxalignmenterror;
	u64 rxrunterror;
	u64 rxjabbererror;
	u64 rxundersize_g;
	u64 rxoversize_g;
	u64 rx64octets_gb;
	u64 rx65to127octets_gb;
	u64 rx128to255octets_gb;
	u64 rx256to511octets_gb;
	u64 rx512to1023octets_gb;
	u64 rx1024tomaxoctets_gb;
	u64 rxlengtherror;
	u64 rxoutofrangetype;
	u64 rxpauseframes;
	u64 rxfifooverflow;
	u64 rxwatchdogerror;
	u64 rxrcverror;
	u64 rxctrlframes_g;
	u64 rxvlanframes_g;
};

struct gmac_priv {
	GMAC_COMMON_STRUCT;
#ifdef FPGA
	struct mii_bus *mii;
#endif
	struct clk *rgmii_clk;
	struct gmac_stats hwstats;
	spinlock_t stats_lock;
	int sbd_irq;
};

#ifdef FPGA
static int gmac_mdio_wait(struct gmac_priv *priv)
{
	unsigned long timeout = jiffies + HZ;

	do {
		if (!(reg_read(priv, GMAC_MII_ADDR) & MII_GMAC_BUSY))
			return 0;

		cond_resched();
	} while (time_after(timeout, jiffies));

	return -ETIMEDOUT;
}

static int gmac_mdio_read(struct mii_bus *bus, int addr, int regnum)
{
	struct gmac_priv *priv = bus->priv;
	u32 reg;
	int ret;

	ret = gmac_mdio_wait(priv);
	if (ret)
		return ret;

	reg = FIELD_PREP(MII_GMAC_PA, addr) | FIELD_PREP(MII_GMAC_RA, regnum) |
	      MII_GMAC_BUSY;
	reg_write(priv, GMAC_MII_ADDR, reg);

	ret = gmac_mdio_wait(priv);
	if (ret)
		return ret;

	return FIELD_GET(MII_DATA_MASK, reg_read(priv, GMAC_MII_DATA));
}

static int gmac_mdio_write(struct mii_bus *bus, int addr, int regnum, u16 val)
{
	struct gmac_priv *priv = bus->priv;
	u32 reg;
	int ret;

	ret = gmac_mdio_wait(priv);
	if (ret)
		return ret;

	reg_write(priv, GMAC_MII_DATA, FIELD_PREP(MII_DATA_MASK, val));

	reg = FIELD_PREP(MII_GMAC_PA, addr) | FIELD_PREP(MII_GMAC_RA, regnum) |
	      MII_GMAC_WRITE | MII_GMAC_BUSY;
	reg_write(priv, GMAC_MII_ADDR, reg);

	return gmac_mdio_wait(priv);
}

static int gmac_mdio_init(struct gmac_priv *priv)
{
	struct device *dev = priv->dev;
	struct device_node *mdio_node, *np = dev->of_node;
	int ret = -ENOMEM;

	mdio_node = of_get_child_by_name(np, "mdio");
	if (!mdio_node)
		return 0;

	priv->mii = devm_mdiobus_alloc(dev);
	if (!priv->mii)
		goto cleanup;

	priv->mii->name = "gmac";
	priv->mii->priv = priv;
	priv->mii->read = gmac_mdio_read;
	priv->mii->write = gmac_mdio_write;
	priv->mii->probe_capabilities = MDIOBUS_C22;
	snprintf(priv->mii->id, MII_BUS_ID_SIZE, "gmac%u_mdio", priv->id);

	ret = devm_of_mdiobus_register(dev, priv->mii, mdio_node);
cleanup:
	of_node_put(mdio_node);
	return ret;
}
#endif

static void gmac_write_mac_addr(struct gmac_priv *priv, const u8 *addr,
				u32 reg)
{
	u32 val;

	/* For MAC Addr registers we have to set the Address Enable (AE)
	 * bit that has no effect on the High Reg 0 where the bit 31 (MO)
	 * is RO.
	 */
	val = GMAC_HI_REG_AE | (addr[5] << 8) | addr[4];
	reg_write(priv, GMAC_ADDR_HIGH(reg), val);
	val = (addr[3] << 24) | (addr[2] << 16) | (addr[1] << 8) | addr[0];
	reg_write(priv, GMAC_ADDR_LOW(reg), val);
}

static void gmac_read_mmc_stats(struct gmac_priv *priv)
{
	unsigned long reg = GMAC_MMC_BASE, i;
	u64 *data = (u64 *)&priv->hwstats;

	for (i = 0; i < ARRAY_SIZE(gmac_gstring_stats); i++, reg += 4) {
		data[i] += reg_read(priv, reg);

		if (reg == GMAC_TXOCTETCOUNT_GB_LOW ||
		    reg == GMAC_TXOCTETCOUNT_G_LOW ||
		    reg == GMAC_RXOCTETCOUNT_GB_LOW ||
		    reg == GMAC_RXOCTETCOUNT_G_LOW) {
			reg += 4;
			data[i] += (u64)reg_read(priv, reg) << 32;
		}
	}
}

static void gmac_mmc_clear(struct gmac_priv *priv)
{
	unsigned long reg;

	/* Clear the MMC registers by reading them */
	for (reg = GMAC_MMC_BASE; reg <= GMAC_MMC_END; reg += 4)
		reg_read(priv, reg);
}

static int gmac_open(struct net_device *dev)
{
	struct gmac_priv *priv = netdev_priv(dev);
	int ret;

	ret = phylink_of_phy_connect(priv->phylink, priv->dev->of_node, 0);
	if (ret)
		return ret;

	ret = xgmac_dma_open(priv->dma, dev, priv->id);
	if (ret) {
		phylink_disconnect_phy(priv->phylink);
		return ret;
	}

	phylink_start(priv->phylink);
	netif_tx_start_all_queues(dev);

	return 0;
}

static int gmac_stop(struct net_device *dev)
{
	struct gmac_priv *priv = netdev_priv(dev);

	phylink_stop(priv->phylink);
	netif_tx_stop_all_queues(dev);
	phylink_disconnect_phy(priv->phylink);

	return xgmac_dma_stop(priv->dma, dev, priv->id);
}

static void gmac_set_rx_mode(struct net_device *dev)
{
	struct gmac_priv *priv = netdev_priv(dev);
	unsigned int value = 0;
	unsigned int perfect_addr_number = 15;
	u32 mc_filter[8] = {};
	int mcbitslog2 = 8, i;

	pr_debug("%s: # mcasts %d, # unicast %d\n", __func__,
		 netdev_mc_count(dev), netdev_uc_count(dev));

	if (dev->flags & IFF_PROMISC) {
		value = GMAC_FRAME_FILTER_PR | GMAC_FRAME_FILTER_PCF;
	} else if (dev->flags & IFF_ALLMULTI) {
		value = GMAC_FRAME_FILTER_PM;	/* pass all multi */
	} else if (!netdev_mc_empty(dev)) {
		struct netdev_hw_addr *ha;

		/* Hash filter for multicast */
		value = GMAC_FRAME_FILTER_HMC;

		netdev_for_each_mc_addr(ha, dev) {
			/* The upper n bits of the calculated CRC are used to
			 * index the contents of the hash table. The number of
			 * bits used depends on the hardware configuration
			 * selected at core configuration time.
			 */
			u32 bit_nr = bitrev32(~crc32_le(~0, ha->addr,
					      ETH_ALEN)) >>
					      (32 - mcbitslog2);
			/* The most significant bit determines the register to
			 * use (H/L) while the other 5 bits determine the bit
			 * within the register.
			 */
			mc_filter[bit_nr >> 5] |= 1 << (bit_nr & 31);
		}
	}

	value |= GMAC_FRAME_FILTER_HPF;
	for (i = 0; i < ARRAY_SIZE(mc_filter); i++)
		reg_write(priv, GMAC_EXTHASH_BASE + i * 4, mc_filter[i]);

	/* Handle multiple unicast addresses (perfect filtering) */
	if (netdev_uc_count(dev) > perfect_addr_number)
		/* Switch to promiscuous mode if more than unicast
		 * addresses are requested than supported by hardware.
		 */
		value |= GMAC_FRAME_FILTER_PR;
	else {
		int reg = 1;
		struct netdev_hw_addr *ha;

		netdev_for_each_uc_addr(ha, dev) {
			gmac_write_mac_addr(priv, ha->addr, reg);
			reg++;
		}

		while (reg <= perfect_addr_number) {
			reg_write(priv, GMAC_ADDR_HIGH(reg), 0);
			reg_write(priv, GMAC_ADDR_LOW(reg), 0);
			reg++;
		}
	}

#ifdef FRAME_FILTER_DEBUG
	/* Enable Receive all mode (to debug filtering_fail errors) */
	value |= GMAC_FRAME_FILTER_RA;
#endif
	reg_write(priv, GMAC_FRAME_FILTER, value);
}

static int gmac_set_mac_address(struct net_device *dev, void *p)
{
	struct gmac_priv *priv = netdev_priv(dev);
	int ret;

	ret = eth_mac_addr(dev, p);
	if (ret)
		return ret;

	gmac_write_mac_addr(priv, dev->dev_addr, 0);

	return 0;
}

static int gmac_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	struct gmac_priv *priv = netdev_priv(dev);

	return phylink_mii_ioctl(priv->phylink, ifr, cmd);
}

static int gmac_set_features(struct net_device *dev,
			     netdev_features_t features)
{
	struct gmac_priv *priv = netdev_priv(dev);
	u32 ctrl = reg_read(priv, GMAC_CONTROL);

	if (features & NETIF_F_LOOPBACK) {
		if (!(ctrl & GMAC_CONTROL_LM)) {
			netdev_info(dev, "MAC internal loopback enabled\n");
			ctrl |= GMAC_CONTROL_LM;
		}
	} else {
		if (ctrl & GMAC_CONTROL_LM) {
			netdev_info(dev, "MAC internal loopback disabled\n");
			ctrl &= ~GMAC_CONTROL_LM;
		}
	}

	if (features & NETIF_F_RXFCS) {
		if (ctrl & (GMAC_CONTROL_ACS | GMAC_CONTROL_CST)) {
			netdev_info(dev, "MAC FCS stripping disabled\n");
			ctrl &= ~(GMAC_CONTROL_ACS | GMAC_CONTROL_CST);
		}
	} else {
		if (!(ctrl & (GMAC_CONTROL_ACS | GMAC_CONTROL_CST))) {
			netdev_info(dev, "MAC FCS stripping enabled\n");
			ctrl |= GMAC_CONTROL_ACS | GMAC_CONTROL_CST;
		}
	}

	reg_write(priv, GMAC_CONTROL, ctrl);

	return 0;
}

static int gmac_change_mtu(struct net_device *dev, int mtu)
{
	struct gmac_priv *priv = netdev_priv(dev);
	unsigned int len = mtu + VLAN_ETH_HLEN + ETH_FCS_LEN;

	reg_write(priv, GMAC_WDT, GMAC_WDT_EN | FIELD_PREP(GMAC_WDT_LEN, len));

	dev->mtu = mtu;

	return 0;
}

static void gmac_get_stats64(struct net_device *dev,
			     struct rtnl_link_stats64 *stats)
{
	struct gmac_priv *priv = netdev_priv(dev);
	struct gmac_stats *hwstats = &priv->hwstats;
	u64 rx_packets_good, tx_packets_good;

	spin_lock(&priv->stats_lock);

	gmac_read_mmc_stats(priv);
	rx_packets_good = hwstats->rxunicastframes_g +
			  hwstats->rxmulticastframes_g;
	tx_packets_good = hwstats->txunicastframes_g +
			  hwstats->txmulticastframes_g +
			  hwstats->txbroadcastframes_g;

	stats->rx_packets = rx_packets_good;
	stats->tx_packets = hwstats->txframecount_gb;
	stats->rx_bytes = hwstats->rxoctetcount_g;
	stats->tx_bytes = hwstats->txoctetcount_g;
	stats->rx_errors = hwstats->rxframecount_gb - rx_packets_good;
	stats->tx_errors = stats->tx_packets - tx_packets_good;
	stats->rx_dropped = dev->stats.rx_dropped;
	stats->tx_dropped = dev->stats.tx_dropped;
	stats->multicast = hwstats->rxmulticastframes_g;
	stats->collisions = hwstats->txlatecol + hwstats->txexcesscol;
	stats->rx_length_errors = hwstats->rxlengtherror +
				  hwstats->rxoutofrangetype;
	stats->rx_over_errors = hwstats->rxfifooverflow;
	stats->rx_crc_errors = hwstats->rxcrcerror;
	stats->rx_frame_errors = hwstats->rxalignmenterror;
	stats->tx_aborted_errors = hwstats->txexcesscol + hwstats->txexcessdef;
	stats->tx_carrier_errors = hwstats->txcarriererror;
	stats->tx_fifo_errors = hwstats->txunderflowerror;
	stats->tx_window_errors = hwstats->txlatecol;

	spin_unlock(&priv->stats_lock);
}

static int gmac_get_phys_port_id(struct net_device *dev,
					  struct netdev_phys_item_id *ppid)
{
	struct gmac_priv *priv = netdev_priv(dev);

	ppid->id[0] = priv->id;
	ppid->id_len = 1;

	return 0;
}

static int gmac_get_port_parent_id(struct net_device *dev,
					  struct netdev_phys_item_id *ppid)
{
	ppid->id[0] = SF_GMAC_DUNMMY_ID;
	ppid->id_len = 1;

	return 0;
}

static void gmac_neigh_destroy(struct net_device *dev,
				      struct neighbour *n)
{
	// struct gmac_priv *priv = netdev_priv(dev);

	/** TODO: call dpns->ops->port_neigh_destroy(dp_port, n); */
	return;
}

static const struct net_device_ops gmac_netdev_ops = {
	.ndo_open		= gmac_open,
	.ndo_stop		= gmac_stop,
	.ndo_start_xmit		= xgmac_dma_xmit_fast,
	.ndo_set_rx_mode	= gmac_set_rx_mode,
	.ndo_set_mac_address	= gmac_set_mac_address,
	.ndo_do_ioctl		= gmac_ioctl,
	.ndo_set_features	= gmac_set_features,
	.ndo_change_mtu		= gmac_change_mtu,
	.ndo_get_stats64	= gmac_get_stats64,
	.ndo_get_phys_port_id	= gmac_get_phys_port_id,
	.ndo_get_port_parent_id	= gmac_get_port_parent_id,
	.ndo_neigh_destroy	= gmac_neigh_destroy,
};


static void gmac_validate(struct phylink_config *config,
			  unsigned long *supported,
			  struct phylink_link_state *state)
{
	__ETHTOOL_DECLARE_LINK_MODE_MASK(mac_supported) = {};

	phylink_set(mac_supported, 10baseT_Half);
	phylink_set(mac_supported, 10baseT_Full);
	phylink_set(mac_supported, 100baseT_Half);
	phylink_set(mac_supported, 100baseT_Full);
	phylink_set(mac_supported, 1000baseT_Half);
	phylink_set(mac_supported, 1000baseT_Full);
	phylink_set(mac_supported, 1000baseKX_Full);
	phylink_set(mac_supported, Autoneg);
	phylink_set(mac_supported, Pause);
	phylink_set(mac_supported, Asym_Pause);
	phylink_set_port_modes(mac_supported);

	linkmode_and(supported, supported, mac_supported);
	linkmode_and(state->advertising, state->advertising, mac_supported);
}

static void gmac_mac_pcs_get_state(struct phylink_config *config,
				   struct phylink_link_state *state)
{
	struct gmac_priv *priv = netdev_priv(to_net_dev(config->dev));
	u32 status;

	status = reg_read(priv, GMAC_RGSMIIIS);

	switch (FIELD_GET(GMAC_RGSMIIIS_SPEED, status)) {
	case 0:
		state->speed = SPEED_10;
		break;
	case 1:
		state->speed = SPEED_100;
		break;
	case 2:
		state->speed = SPEED_1000;
		break;
	default:
		break;
	}

	state->link = !!(status & GMAC_RGSMIIIS_LNKSTS);
	state->duplex = status & GMAC_RGSMIIIS_LNKMODE ?
			DUPLEX_FULL : DUPLEX_HALF;
	state->pause = MLO_PAUSE_TX | MLO_PAUSE_RX;
}

static void gmac_mac_config(struct phylink_config *config, unsigned int mode,
			    const struct phylink_link_state *state)
{
}

static void gmac_mac_an_restart(struct phylink_config *config)
{
	/* Not supported */
}

static void gmac_mac_link_down(struct phylink_config *config,
			       unsigned int mode, phy_interface_t interface)
{
	struct gmac_priv *priv = netdev_priv(to_net_dev(config->dev));

	reg_clear(priv, GMAC_CONTROL, GMAC_CONTROL_TE | GMAC_CONTROL_RE);

	reg_write(priv, GPI_TRANS_CTRL, TX_CLOSE_EN);

	if (phy_interface_mode_is_rgmii(interface))
		clk_disable(priv->rgmii_clk);
}

static void gmac_mac_link_up(struct phylink_config *config,
			     struct phy_device *phy, unsigned int mode,
			     phy_interface_t interface, int speed, int duplex,
			     bool tx_pause, bool rx_pause)
{
	struct gmac_priv *priv = netdev_priv(to_net_dev(config->dev));
	u32 ctrl, fc;

	if (phy_interface_mode_is_rgmii(interface)) {
		/* Make sure RGMII clock is up and stable before writing to
		 * GMAC_CONTROL register.
		 */
		clk_enable(priv->rgmii_clk);
		fsleep(1);
	}

	ctrl = reg_read(priv, GMAC_CONTROL);
	ctrl |= GMAC_CONTROL_TE | GMAC_CONTROL_RE;

	ctrl &= ~(GMAC_CONTROL_PS | GMAC_CONTROL_FES);
	switch (speed) {
	case SPEED_1000:
		break;
	case SPEED_100:
		ctrl |= GMAC_CONTROL_PS | GMAC_CONTROL_FES;
		break;
	case SPEED_10:
		ctrl |= GMAC_CONTROL_PS;
		break;
	default:
		return;
	}

	if (duplex == DUPLEX_FULL)
		ctrl |= GMAC_CONTROL_DM;
	else
		ctrl &= ~GMAC_CONTROL_DM;

	fc = GMAC_FLOW_CTRL_UP;
	if (tx_pause)
		fc |= GMAC_FLOW_CTRL_TFE;
	if (rx_pause)
		fc |= GMAC_FLOW_CTRL_RFE;
	if (duplex == DUPLEX_FULL)
		fc |= FIELD_PREP(GMAC_FLOW_CTRL_PT_MASK, 0x400);

	reg_write(priv, GMAC_FLOW_CTRL, fc);
	reg_write(priv, GMAC_CONTROL, ctrl);

	if (interface == PHY_INTERFACE_MODE_QSGMII) {
		regmap_clear_bits(priv->ethsys, ETHSYS_RATIO_LOAD, BIT(priv->id));
		regmap_set_bits(priv->ethsys, ETHSYS_RATIO_LOAD, BIT(priv->id));
	}

	reg_clear(priv, GPI_TRANS_CTRL, TX_CLOSE_EN);
}

static const struct phylink_mac_ops gmac_phylink_mac_ops = {
	.validate	= gmac_validate,
	.mac_pcs_get_state	= gmac_mac_pcs_get_state,
	.mac_config	= gmac_mac_config,
	.mac_an_restart	= gmac_mac_an_restart,
	.mac_link_down	= gmac_mac_link_down,
	.mac_link_up	= gmac_mac_link_up,
};

static int gmac_soft_reset(struct gmac_priv *priv)
{
	int timeout = 10000;

	reg_write(priv, GPI_TRANS_CTRL, GMAC_RESET);
	while (reg_read(priv, GPI_TRANS_CTRL)) {
		if (!timeout--)
			return -ETIMEDOUT;

		cpu_relax();
	}

	return 0;
}

static int gmac_ethtool_nway_reset(struct net_device *dev)
{
	struct gmac_priv *priv = netdev_priv(dev);

	return phylink_ethtool_nway_reset(priv->phylink);
}

static void gmac_ethtool_get_pauseparam(struct net_device *dev,
					struct ethtool_pauseparam *pause)
{
	struct gmac_priv *priv = netdev_priv(dev);

	phylink_ethtool_get_pauseparam(priv->phylink, pause);
}

static int gmac_ethtool_set_pauseparam(struct net_device *dev,
				       struct ethtool_pauseparam *pause)
{
	struct gmac_priv *priv = netdev_priv(dev);

	return phylink_ethtool_set_pauseparam(priv->phylink, pause);
}

static void gmac_ethtool_get_strings(struct net_device *dev, u32 stringset,
				     u8 *data)
{
	u32 i;

	if (stringset != ETH_SS_STATS)
		return;

	for (i = 0; i < ARRAY_SIZE(gmac_gstring_stats); i++) {
		strncpy((char *)data, gmac_gstring_stats[i], ETH_GSTRING_LEN);
		data += ETH_GSTRING_LEN;
	}
}

static void gmac_ethtool_get_stats(struct net_device *dev,
				   struct ethtool_stats *stats, u64 *data)
{
	struct gmac_priv *priv = netdev_priv(dev);

	spin_lock(&priv->stats_lock);

	gmac_read_mmc_stats(priv);
	memcpy(data, &priv->hwstats, sizeof(priv->hwstats));

	spin_unlock(&priv->stats_lock);
}

static int gmac_ethtool_reset(struct net_device *dev, u32 *flags)
{
	struct gmac_priv *priv = netdev_priv(dev);
	int ret = 0;

	if (*flags & ETH_RESET_MGMT) {
		spin_lock(&priv->stats_lock);

		gmac_read_mmc_stats(priv);
		memset(&priv->hwstats, 0, sizeof(priv->hwstats));
		memset(&dev->stats, 0, sizeof(dev->stats));

		spin_unlock(&priv->stats_lock);

		*flags &= ~ETH_RESET_MGMT;
	}
	if (*flags & ETH_RESET_MAC) {
		u32 reg;

		disable_irq(priv->sbd_irq);
		clk_enable(priv->rgmii_clk);
		fsleep(1);
		ret = gmac_soft_reset(priv);
		if (ret)
			goto out;

		/* Reconfigure core register */
		reg_write(priv, GMAC_INT_MASK, ~GMAC_INT_DISABLE_RGMII);
		reg = GMAC_CORE_INIT;
		if (!(dev->features & NETIF_F_RXFCS))
			reg |= GMAC_CONTROL_ACS | GMAC_CONTROL_CST;

		if (dev->features & NETIF_F_LOOPBACK)
			reg |= GMAC_CONTROL_LM;

		reg_write(priv, GMAC_CONTROL, GMAC_CORE_INIT);

		/* Rewrite MAC address list */
		gmac_write_mac_addr(priv, dev->dev_addr, 0);
		gmac_set_rx_mode(dev);

		/* MRU */
		gmac_change_mtu(dev, dev->mtu);

		/* Force a link down event */
		phylink_mac_change(priv->phylink, false);

		*flags &= ~ETH_RESET_MAC;

out:
		clk_disable(priv->rgmii_clk);
		enable_irq(priv->sbd_irq);
	}

	return ret;
}

static int gmac_ethtool_get_sset_count(struct net_device *dev, int stringset)
{
	switch (stringset) {
	case ETH_SS_STATS:
		return ARRAY_SIZE(gmac_gstring_stats);
	default:
		return -EOPNOTSUPP;
	}
}

static int gmac_ethtool_get_link_ksettings(struct net_device *dev,
					   struct ethtool_link_ksettings *cmd)
{
	struct gmac_priv *priv = netdev_priv(dev);

	return phylink_ethtool_ksettings_get(priv->phylink, cmd);
}

static int gmac_ethtool_set_link_ksettings(struct net_device *dev,
					   const struct ethtool_link_ksettings *cmd)
{
	struct gmac_priv *priv = netdev_priv(dev);

	return phylink_ethtool_ksettings_set(priv->phylink, cmd);
}

static const struct ethtool_ops gmac_ethtool_ops = {
	.nway_reset		= gmac_ethtool_nway_reset,
	.get_link		= ethtool_op_get_link,
	.get_pauseparam		= gmac_ethtool_get_pauseparam,
	.set_pauseparam		= gmac_ethtool_set_pauseparam,
	.get_strings		= gmac_ethtool_get_strings,
	.get_ethtool_stats	= gmac_ethtool_get_stats,
	.reset			= gmac_ethtool_reset,
	.get_sset_count		= gmac_ethtool_get_sset_count,
	.get_link_ksettings	= gmac_ethtool_get_link_ksettings,
	.set_link_ksettings	= gmac_ethtool_set_link_ksettings,
};

static irqreturn_t gmac_irq(int irq, void *dev_id)
{
	struct gmac_priv *priv = dev_id;
	u32 status = reg_read(priv, GMAC_INT_STATUS);

	if (!(status & GMAC_INT_STATUS_RSGMII))
		return IRQ_NONE;

	/* Ack link interrupt */
	status = reg_read(priv, GMAC_RGSMIIIS);
	phylink_mac_change(priv->phylink, status & GMAC_RGSMIIIS_LNKSTS);

	return IRQ_HANDLED;
}

static int gmac_irq_setup(struct gmac_priv *priv)
{
	struct device *dev = priv->dev;
	const char *irq_name;

	/* Only enable link change interrupts. TODO: handle LPI */
	reg_write(priv, GMAC_INT_MASK, ~GMAC_INT_DISABLE_RGMII);

	irq_name = devm_kasprintf(dev, GFP_KERNEL, "gmac%u_sbd", priv->id);
	if (!irq_name)
		return -ENOMEM;

	/* Shared with PHY IRQ */
	return devm_request_irq(dev, priv->sbd_irq, gmac_irq,
				IRQF_SHARED | IRQF_ONESHOT, irq_name, priv);
}


static int gmac_rgmii_delay(struct gmac_priv *priv, phy_interface_t phy_mode)
{
	u32 reg = 0, rxd = MAC5_DELAY_DEFAULT, txd = MAC5_DELAY_DEFAULT;

	of_property_read_u32(priv->dev->of_node, "rx-internal-delay-ps", &rxd);
	of_property_read_u32(priv->dev->of_node, "tx-internal-delay-ps", &txd);

	switch (phy_mode) {
	case PHY_INTERFACE_MODE_RGMII_TXID:
		txd = 0;
		break;
	case PHY_INTERFACE_MODE_RGMII_RXID:
		rxd = 0;
		break;
	case PHY_INTERFACE_MODE_RGMII_ID:
		txd = 0;
		rxd = 0;
		break;
	default:
		break;
	}

	rxd = DIV_ROUND_CLOSEST(rxd, MAC5_DELAY_STEP);
	txd = DIV_ROUND_CLOSEST(txd, MAC5_DELAY_STEP);

	if (rxd > 256 || txd > 256)
		return -EINVAL;

	if (rxd)
		reg |= FIELD_PREP(MAC5_RX_DELAY, rxd - 1) | MAC5_RX_DELAY_EN;

	if (txd)
		reg |= FIELD_PREP(MAC5_TX_DELAY, txd - 1) | MAC5_TX_DELAY_EN;

	return regmap_update_bits(priv->ethsys, ETHSYS_MAC(4),
				  MAC5_DELAY_MASK, reg);
}


static int gmac_phy_setup(struct gmac_priv *priv)
{
	struct device *dev = priv->dev;
	struct device_node *np = dev->of_node;
	phy_interface_t phy_mode;
	struct phylink *phylink;
	int ret;

	ret = of_get_phy_mode(np, &phy_mode);
	if (ret)
		return ret;

	if (phy_interface_mode_is_rgmii(phy_mode)) {
		ret = gmac_rgmii_delay(priv, phy_mode);
		if (ret)
			return ret;

		priv->rgmii_clk = devm_clk_get(dev, "rgmii");
		if (IS_ERR(priv->rgmii_clk))
			return PTR_ERR(priv->rgmii_clk);

		ret = clk_prepare_enable(priv->rgmii_clk);
		if (ret)
			return ret;

		/* After the clock becomes stable (~20 cycles), it can be
		 * gated off to save power.
		 * It will be turned on again in gmac_mac_link_up.
		 */
		fsleep(1);
		clk_disable(priv->rgmii_clk);
	}

	priv->phylink_config.dev = &priv_to_netdev(priv)->dev;
	priv->phylink_config.type = PHYLINK_NETDEV;

	phylink = phylink_create(&priv->phylink_config, dev->fwnode, phy_mode,
				 &gmac_phylink_mac_ops);
	if (IS_ERR(phylink))
		return PTR_ERR(phylink);

	priv->phylink = phylink;
	return 0;
}

static int gmac_probe(struct platform_device *pdev)
{
	struct platform_device *dma_pdev;
	struct device_node *dma_node;
	struct net_device *ndev;
	struct gmac_priv *priv;
	struct resource *r;
	int ret;

	dma_node = of_parse_phandle(pdev->dev.of_node, "dmas", 0);
	if (!dma_node)
		return -ENODEV;

	dma_pdev = of_find_device_by_node(dma_node);
	of_node_put(dma_node);
	if (!dma_pdev)
		return -ENODEV;

	ndev = devm_alloc_etherdev_mqs(&pdev->dev, sizeof(*priv), DMA_CH_MAX,
				       DMA_CH_MAX);
	if (!ndev)
		return -ENOMEM;

	SET_NETDEV_DEV(ndev, &pdev->dev);
	platform_set_drvdata(pdev, ndev);
	priv = netdev_priv(ndev);
	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->ioaddr = devm_ioremap_resource(&pdev->dev, r);
	if (IS_ERR(priv->ioaddr))
		return PTR_ERR(priv->ioaddr);

	priv->csr_clk = devm_clk_get(&pdev->dev, "csr");
	if (IS_ERR(priv->csr_clk))
		return PTR_ERR(priv->csr_clk);

	priv->id = offset_to_id(r->start);
	priv->ethsys = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
						       "ethsys");
	if (IS_ERR(priv->ethsys))
		return PTR_ERR(priv->ethsys);

	ret = regmap_set_bits(priv->ethsys, ETHSYS_RST, BIT(priv->id));
	if (ret)
		return ret;

	ret = clk_prepare_enable(priv->csr_clk);
	if (ret)
		return ret;

	priv->dev = &pdev->dev;
	priv->dma = platform_get_drvdata(dma_pdev);
	spin_lock_init(&priv->stats_lock);
	ndev->netdev_ops = &gmac_netdev_ops;
	ndev->ethtool_ops = &gmac_ethtool_ops;
	/* TODO: support more features */
	ndev->features = NETIF_F_RXHASH | NETIF_F_GRO | NETIF_F_SG |
			 NETIF_F_LLTX | NETIF_F_HW_TC;
	ndev->hw_features = (ndev->features & ~NETIF_F_RXHASH) |
			    NETIF_F_LOOPBACK | NETIF_F_RXFCS |
				NETIF_F_HW_L2FW_DOFFLOAD;
	ndev->vlan_features = ndev->features;
	ndev->priv_flags |= IFF_UNICAST_FLT | IFF_LIVE_ADDR_CHANGE;
	ndev->max_mtu = MAX_FRAME_SIZE - VLAN_ETH_HLEN - ETH_FCS_LEN;

	ret = gmac_soft_reset(priv);
	if (ret)
		goto out_disable_clk;
#ifdef FPGA
	ret = gmac_mdio_init(priv);
	if (ret)
		goto out_disable_clk;
#endif
	ret = of_get_mac_address(pdev->dev.of_node, ndev->dev_addr);
	if (ret) {
		eth_hw_addr_random(ndev);
		dev_warn(&pdev->dev, "generated random MAC address %pM\n",
			 ndev->dev_addr);
	}
	gmac_write_mac_addr(priv, ndev->dev_addr, 0);

	ret = platform_get_irq(pdev, 0);
	if (ret < 0)
		goto out_disable_clk;
	priv->sbd_irq = ret;

	ret = gmac_phy_setup(priv);
	if (ret)
		goto out_disable_clk;

	ret = gmac_irq_setup(priv);
	if (ret)
		goto out_phy_cleanup;

	reg_write(priv, GMAC_CONTROL, GMAC_CORE_INIT);

	gmac_mmc_clear(priv);
	reg_write(priv, GMAC_WDT, GMAC_WDT_EN |
		  FIELD_PREP(GMAC_WDT_LEN, VLAN_ETH_FRAME_LEN + ETH_FCS_LEN));

	ret = register_netdev(ndev);
	if (ret)
		goto out_phy_cleanup;

	return 0;
out_phy_cleanup:
	phylink_destroy(priv->phylink);
out_disable_clk:
	clk_unprepare(priv->rgmii_clk);
	clk_disable_unprepare(priv->csr_clk);
	return ret;
}

static int gmac_remove(struct platform_device *pdev)
{
	struct net_device *dev = platform_get_drvdata(pdev);
	struct gmac_priv *priv = netdev_priv(dev);

	unregister_netdev(dev);
	phylink_destroy(priv->phylink);
	clk_unprepare(priv->rgmii_clk);
	clk_disable_unprepare(priv->csr_clk);

	return regmap_clear_bits(priv->ethsys, ETHSYS_RST, BIT(priv->id));
}

static const struct of_device_id gmac_match[] = {
	{ .compatible = "siflower,gmac" },
	{},
};
MODULE_DEVICE_TABLE(of, gmac_match);

static struct platform_driver gmac_driver = {
	.probe	= gmac_probe,
	.remove	= gmac_remove,
	.driver	= {
		.name		= "sfgmac",
		.of_match_table	= gmac_match,
	},
};
module_platform_driver(gmac_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Qingfang Deng <qingfang.deng@siflower.com.cn>");
MODULE_DESCRIPTION("Ethernet GMAC driver for SoC");
