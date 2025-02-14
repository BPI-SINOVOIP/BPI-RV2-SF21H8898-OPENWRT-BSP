/*
* Description
*
* Copyright (C) 2016-2020 Qin.Xia <qin.xia@siflower.com.cn>
*
* Siflower software
*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/bitmap.h>
#include <linux/delay.h>
#include <linux/mdio.h>
#include <linux/phy.h>
#include <linux/of_mdio.h>
#include <linux/gpio/consumer.h>
#include <linux/of_irq.h>
#include "sf_eswitch.h"
#include "intel7084_src/src/gsw_sw_init.h"
#ifdef CONFIG_DEBUG_FS
#include "sf_eswitch_debug.h"
#endif


struct mutex op_switch_lock;
// spinlock_t	mdio_lock;
struct mii_bus *gp_mii_bus = NULL;
struct vlan_entry vlan_entries;
extern ethsw_api_dev_t *pedev0[GSW_DEV_MAX];

#ifdef CONFIG_DEBUG_FS
static struct file_operations esw_debug_ops = {
	.owner = THIS_MODULE,
	.open  = sf_eswitch_debug_open,
	.read  = sf_eswitch_debug_read,
	.write  = sf_eswitch_debug_write,
	.release  = sf_eswitch_debug_release,
	.llseek  = default_llseek,
	.unlocked_ioctl = sf_eswitch_debug_ioctl
};
#endif

static irqreturn_t sf_eswitch_irq(int irq, void *data)
{
	struct sf_eswitch_priv *pesw_priv = data;
	struct sf_eswitch_api_t *pesw_api = pesw_priv->pesw_api;
	int i, port_mask;

	port_mask = pesw_api->irq_ack();
	if (port_mask < 0)
		return IRQ_NONE;

	port_mask &= pesw_priv->port_list;
	for (i = 0; port_mask; i++, port_mask = (unsigned)port_mask >> 1)
	{
		if (!(1 & port_mask))
			continue;

		if (pesw_api->check_phy_linkup(i))
			pesw_priv->phy_status |= BIT(i);
		else
			pesw_priv->phy_status &= ~BIT(i);
	}

	if (pesw_api->set_rgmii)
		pesw_api->set_rgmii(pesw_priv->phy_status);

	return IRQ_HANDLED;
}

void sf_eswitch_deinit_swdev(struct mdio_device *mdiodev) {
	struct sf_eswitch_priv *pesw_priv = dev_get_drvdata(&mdiodev->dev);

	pesw_priv->pesw_api->vender_deinit(pesw_priv);
#ifdef CONFIG_SWCONFIG
	unregister_switch(&pesw_priv->swdev);
#endif
	return;
}

unsigned char sf_eswitch_init_swdev(struct mdio_device *mdiodev, struct mii_bus* pmii_bus) {
	struct sf_eswitch_priv *pesw_priv = dev_get_drvdata(&mdiodev->dev);
	struct device_node *mdio_node = NULL;
#ifdef CONFIG_SWCONFIG
	struct switch_dev *pswdev;
	int ret = 0;
#endif
	unsigned int chip_id = 0, retry_times = 0;
	gp_mii_bus = pmii_bus;

	if(gp_mii_bus == NULL){
		return UNKNOWN;
	}
#ifdef CONFIG_SWCONFIG
	pswdev = &pesw_priv->swdev;
#endif
	ethsw_init_pedev0();
	pedev0[0]->mdio_addr = mdiodev->addr;
	do{
		//chip id to read intel7084&intel7082

		intel7084_mdio_rd(0xFA11, 0, 16, &chip_id);
		if(chip_id == INTEL7084_ID || chip_id == INTEL7082_ID){
			if(chip_id == INTEL7084_ID)
				pesw_priv->model = INTEL7084;
			else if(chip_id == INTEL7082_ID)
				pesw_priv->model = INTEL7082;

			pesw_priv->pesw_api = &intel7084_api;
			pesw_priv->port_list = SWITCH_PORT_LIST;

#ifdef CONFIG_SWCONFIG
			pswdev->ports = INTEL_SWITCH_PORT_NUM;
			pswdev->cpu_port = RGMII_PORT0;
#endif
			break;
		}

		retry_times++;
		printk("unknown switch type! retry times:%d\n", retry_times);

	}while(retry_times < 3);

#ifdef CONFIG_SWCONFIG
	pswdev->ops = pesw_priv->pesw_api->ops;
#endif
	memset(&vlan_entries, 0, sizeof(struct vlan_entry));
	if (!vlan_entries.entry_list.prev)
		INIT_LIST_HEAD(&(vlan_entries.entry_list));
	// init eswitch hw
	pesw_priv->pesw_api->vender_init(pesw_priv);
	// init eswitch led mode
	pesw_priv->pesw_api->led_init(LED_NORMAL);


#ifdef CONFIG_SWCONFIG
	ret = register_switch(pswdev, NULL);
	if (ret) {

		pesw_priv->pesw_api->vender_deinit(pesw_priv);
		printk("failed to register sfax8\n");
	}
#endif
	return pesw_priv->model;

}

unsigned int sf_eswitch_read_phy_reg(struct sf_eswitch_priv* priv , int phyNo, int phyReg) {
	GSW_MDIO_data_t parm;
	unsigned int phyData = 0;

	SF_MDIO_LOCK();
	if (priv->model == INTEL7084 || priv->model == INTEL7082)
	{
		parm.nAddressDev = phyNo;
		parm.nAddressReg = phyReg;
		intel7084_phy_rd(&parm);
		phyData = parm.nData;
	}
	SF_MDIO_UNLOCK();

	return phyData;
}

void sf_eswitch_write_phy_reg(struct sf_eswitch_priv* priv, int phyNo, int phyReg, int phyData) {
	GSW_MDIO_data_t parm;

	SF_MDIO_LOCK();
	if (priv->model == INTEL7084 || priv->model == INTEL7082) {
		parm.nAddressDev = phyNo;
		parm.nAddressReg = phyReg;
		parm.nData = phyData;
		intel7084_phy_wr(&parm);
	}
	SF_MDIO_UNLOCK();

	return;
}

int mdio_read_ext(int phyaddr, int phyreg, int *phydata) {
	struct mii_bus *pmii_bus ;
	int phy_value= 0;
	pmii_bus = gp_mii_bus;
	if(!pmii_bus){
		printk("mdio bus not found\n");
		return -1;
	}

	phy_value = mdiobus_read(pmii_bus, phyaddr, phyreg);
	if (phy_value < 0)
		return -1;
	*phydata = phy_value;
	return 0;
}

int mdio_write_ext(int phyaddr, int phyreg, int phydata) {
	struct mii_bus *pmii_bus;
	pmii_bus = gp_mii_bus;
	if(!pmii_bus){
		printk("mdio bus not found\n");
		return -1;
	}
	mdiobus_write(pmii_bus, phyaddr, phyreg, phydata);
	return 0;
}

static int sf_eswitch_init(struct mdio_device *mdiodev) {

	struct sf_eswitch_priv *pesw_priv = dev_get_drvdata(&mdiodev->dev);
	int err;


	if (pesw_priv->pesw_api->irq_init) {
		int irq = of_irq_get(mdiodev->dev.of_node, 0);

		if (irq == -EPROBE_DEFER) {
			return irq;
		} else if (irq > 0) {
			err = pesw_priv->pesw_api->irq_init(irq_get_trigger_type(irq));
			if (err)
				goto err_out;

			err = devm_request_threaded_irq(&mdiodev->dev, irq, NULL,
							sf_eswitch_irq, IRQF_ONESHOT,
							"sf_eswitch", pesw_priv);
			if (err)
				goto err_out;
		}
	}

	// if (pesw_priv->model == INTEL7084)
	// 	sf_intel7084_qos_register();

	printk("eswitch init success\n");
	return 0;

err_out:
	// pesw_priv->pesw_api->vender_deinit(pesw_priv);
	printk("eswitch init fail\n");
	return err;
}

static int sf_eswitch_deinit(struct mdio_device *mdiodev)
{
	struct sf_eswitch_priv *pesw_priv = dev_get_drvdata(&mdiodev->dev);
	//	struct sf_eswitch_api_t *pesw_api = pesw_priv->pesw_api;
	if(!pesw_priv){
		printk("eswitch is null deinit fail\n");
		return -1;
	}

	// if (pesw_priv->model == INTEL7084)
	// 	sf_intel7084_qos_unregister();

	// pesw_api->vender_deinit(pesw_priv);

	printk("eswitch deinit success\n");
	return 0;
}

static int sf_eswitch_probe(struct mdio_device *mdiodev) {
	struct sf_eswitch_priv *pesw_priv;
	int err;

	pesw_priv = devm_kzalloc(&mdiodev->dev, sizeof(struct sf_eswitch_priv),
				 GFP_KERNEL);
	if (!pesw_priv) {
		dev_err(&mdiodev->dev, "no memory for eswitch data\n");
		err = -ENOMEM;
		goto err_out;
	}
	mdiodev->reset_gpio = devm_gpiod_get_optional(&mdiodev->dev, "reset",
						      GPIOD_OUT_HIGH);
	if (IS_ERR(mdiodev->reset_gpio)) {
		return PTR_ERR(mdiodev->reset_gpio);
	} else if (mdiodev->reset_gpio) {
		msleep(250);
		gpiod_set_value_cansleep(mdiodev->reset_gpio, 0);
		msleep(1000);
	}

	dev_set_drvdata(&mdiodev->dev, pesw_priv);
	pesw_priv->bus = mdiodev->bus;
	pesw_priv->dev = &mdiodev->dev;

	// spin_lock_init(&mdio_lock);
    mutex_init(&op_switch_lock);
#ifdef CONFIG_SWCONFIG
	pesw_priv->swdev.alias = "sfax8_eswitch";
	pesw_priv->swdev.name = "sfax8_eswitch";
	pesw_priv->swdev.vlans = 4096;
#endif
	pesw_priv->init = sf_eswitch_init;
	pesw_priv->deinit = sf_eswitch_deinit;
	pesw_priv->init_swdev = sf_eswitch_init_swdev;
	pesw_priv->deinit_swdev = sf_eswitch_deinit_swdev;
	pesw_priv->write_phy = sf_eswitch_write_phy_reg;
	pesw_priv->read_phy = sf_eswitch_read_phy_reg;

#ifdef CONFIG_DEBUG_FS
	pesw_priv->esw_debug = debugfs_create_file("esw_debug", 0777, NULL,
			(void *)pesw_priv, &esw_debug_ops);
	if (IS_ERR(pesw_priv->esw_debug)) {
		err = PTR_ERR(pesw_priv->esw_debug);
		goto err_out;
	}
#endif

	pesw_priv->init_swdev(mdiodev, pesw_priv->bus);
	err = pesw_priv->init(mdiodev);
	if (err)
		goto err_out_init;

	printk("eswitch probe success\n");
	return 0;

err_out_init:
	pesw_priv->deinit_swdev(mdiodev);
#ifdef CONFIG_DEBUG_FS
	debugfs_remove(pesw_priv->esw_debug);
#endif
err_out:
	printk("eswitch probe fail\n");
	return err;
}


static void sf_eswitch_remove(struct mdio_device *mdiodev) {
	struct sf_eswitch_priv *pesw_priv = dev_get_drvdata(&mdiodev->dev);

	if(!pesw_priv){
		printk("eswitch is null, remove fail\n");
		return ;
	}

	if (pesw_priv->model == INTEL7084 || pesw_priv->model == INTEL7082)
		intel7084_bridge_redirect_disable();

	pesw_priv->deinit_swdev(mdiodev);
	pesw_priv->deinit(mdiodev);
#ifdef CONFIG_DEBUG_FS
	debugfs_remove(pesw_priv->esw_debug);
#endif

    mutex_destroy(&op_switch_lock);
	printk("eswitch remove success\n");
}

static const struct of_device_id eswitch_match[] = {
    { .compatible = "sf-eswitch"},
    { /* sentinel */ },
};

static struct mdio_driver sf_eswitch_driver = {
	.mdiodrv.driver = {
		.name = "switch4",
		.of_match_table = eswitch_match,
	},
	.probe = sf_eswitch_probe,
	.remove = sf_eswitch_remove,
};


mdio_module_driver(sf_eswitch_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Qin.Xia <qin.xia@siflower.com.cn>");
MODULE_DESCRIPTION("Gigabit switch driver for sfax8");
MODULE_VERSION(SF_VERSION);
