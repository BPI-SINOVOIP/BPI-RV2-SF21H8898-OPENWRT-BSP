#include <linux/netfilter.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/icmp.h>
#include <linux/ip.h>
#include <linux/byteorder/generic.h>
#include <linux/percpu-defs.h>
#include <linux/skbuff.h>
#include <linux/if_arp.h>
#include <linux/if_vlan.h>
#include <linux/minmax.h>
#include <uapi/linux/if_ether.h>
#include <uapi/linux/icmp.h>
#include <uapi/linux/icmpv6.h>
#include <uapi/linux/in.h>
#include <uapi/linux/in6.h>
#include <uapi/linux/ip.h>
#include <uapi/linux/ipv6.h>
#include <uapi/linux/netfilter.h>
#include <uapi/linux/if_arp.h>
#include <uapi/linux/netfilter_ipv4.h>
#include <uapi/linux/udp.h>
#include <net/if_inet6.h>
#include <net/sock.h>
#include <net/ip.h>
#include <net/addrconf.h>
#include <asm/checksum.h>

#include "fast_ping.h"

struct dpns_fast_response_lan_addr_info sf_lan_addr[8] = {0};

static inline void dump_addr(void)
{
	int i;

	for(i = 0; i < 8; i ++){
		printk("%pI4", &sf_lan_addr[i].v4.ip);
		printk("%pI6", sf_lan_addr[i].v6.ip[0]);
		printk("%pI6", sf_lan_addr[i].v6.ip[1]);
	}
}

static inline struct sock *icmp_sk(struct net *net)
{
        return this_cpu_read(*net->ipv4.icmp_sk);
}

static bool is_lan_ip4_addr(__be32 ip4)
{
	int i;

	for (i = 0; i < 8; i++) {
		if (ip4 == sf_lan_addr[i].v4.ip)
			return true;
	}

	return false;
}

static bool is_lan_ip6_addr(struct in6_addr ip6)
{
	int i;

	for(i = 0; i < 8; i++) {
		if (!memcmp(&ip6, sf_lan_addr[i].v6.ip[0], 16) || 
				!memcmp(&ip6, sf_lan_addr[i].v6.ip[1], 16))
			return true;
	}

	return false;
}

static void inline dpns_ip4_make_skb(struct sk_buff *skb)
{
	struct ethhdr *ethhdr;
	struct iphdr *iphdr;
	struct icmphdr *icmphdr;
	struct sock *sk = icmp_sk(dev_net(skb->dev));

	ethhdr = (struct ethhdr*)(skb->data - ETH_HLEN);
	iphdr = get_iphdr(skb);
	icmphdr = (struct icmphdr*)((char *)iphdr + sizeof(*iphdr));

	swap_mac_addr(ethhdr->h_dest, ethhdr->h_source);

	swap(iphdr->saddr, iphdr->daddr);
	iphdr->ttl = (__u8)128;
	iphdr->frag_off = htons(IP_DF);
	ip_select_ident(sock_net(sk), skb, sk);
	iphdr->check = 0;
	iphdr->check = ip_fast_csum(iphdr, iphdr->ihl);

	icmphdr->type = ICMP_ECHOREPLY;
	icmphdr->checksum = (__sum16)0;

	skb->csum_start = (unsigned char *)icmphdr - skb->head;
	skb->csum_offset = 2;
	skb->ip_summed = CHECKSUM_PARTIAL;
}

static void dpns_ip6_make_skb(struct sk_buff *skb)
{
	struct ethhdr *ethhdr;
	struct ipv6hdr *ipv6hdr;
	struct icmp6hdr *icmp6hdr;
	__u32 sum = 0;
	__u16 csum;

	ethhdr = (struct ethhdr *)(skb->data - ETH_HLEN);
	ipv6hdr = get_ipv6hdr(skb);
	icmp6hdr = (struct icmp6hdr*)((char *)ipv6hdr + sizeof(*ipv6hdr));
	swap_mac_addr(ethhdr->h_dest, ethhdr->h_source);

	swap(ipv6hdr->saddr, ipv6hdr->daddr);
	memset(ipv6hdr->flow_lbl, '0', 3);
	ipv6hdr->hop_limit = 64;

	icmp6hdr->icmp6_type = ICMPV6_ECHO_REPLY;
	//IPv6 pseudo-header checksum calculation;
	sum += IPPROTO_ICMPV6;
	sum += ntohs(ipv6hdr->payload_len);
	sum = from32to16(sum);
	csum = ~ntohs((__u16)sum);
	icmp6hdr->icmp6_cksum = csum;
	skb->csum_start = (unsigned char *)icmp6hdr - skb->head - 32;
	skb->csum_offset = 34;
	skb->ip_summed = CHECKSUM_PARTIAL;
}

static void inline dpns_arp_make_skb(struct sk_buff *skb)
{
	struct arphdr *arphdr;
	struct ethhdr *ethhdr;
	struct arp_data *arpdata;
	int i;
	u8 addr_tmp[4];

	ethhdr = (struct ethhdr *)(skb->data - ETH_HLEN);
	arphdr = get_arphdr(skb);
	arpdata = (struct arp_data*)((char*)arphdr + sizeof(*arphdr));

	for (i = 0; i < 8; i ++) {
		if(!memcmp(arpdata->target_ip, &sf_lan_addr[i].v4.ip, 4))
			break;
	}

	memcpy(ethhdr->h_dest, ethhdr->h_source, ETH_ALEN);
	memcpy(ethhdr->h_source, skb->dev->dev_addr, ETH_ALEN);

	arphdr->ar_op = htons(ARPOP_REPLY);
	memcpy(addr_tmp, arpdata->sender_ip, 4);
	memcpy(arpdata->sender_ip, arpdata->target_ip, 4);
	memcpy(arpdata->target_ip, addr_tmp, 4);
	memcpy(arpdata->target_mac, arpdata->sender_mac, ETH_ALEN);
	memcpy(arpdata->sender_mac, sf_lan_addr[i].mac, ETH_ALEN);
	skb->ip_summed = CHECKSUM_COMPLETE;
}

static unsigned int dpns_response_icmp_v4(struct sk_buff *skb)
{
	struct iphdr *iphdr;
	struct icmphdr *icmphdr;

	iphdr = get_iphdr(skb);
	icmphdr = (struct icmphdr*)((char *)iphdr + sizeof(*iphdr));

	if (iphdr->protocol == IPPROTO_ICMP && is_lan_ip4_addr(iphdr->daddr)
					&&icmphdr->type == ICMP_ECHO) {
		if (iphdr->frag_off != 0)  //Do Not Fragment
			return NF_ACCEPT;

		dpns_ip4_make_skb(skb);
		skb_push(skb, ETH_HLEN);
		skb_set_queue_mapping(skb, 2);
		skb->dev->netdev_ops->ndo_start_xmit(skb, skb->dev);

		return NF_STOLEN;
	}

	return NF_ACCEPT;
}

static unsigned int dpns_response_icmp_v6(struct sk_buff *skb)
{
	struct ipv6hdr *ipv6hdr;
	struct icmp6hdr *icmp6hdr;

	ipv6hdr = get_ipv6hdr(skb);
	icmp6hdr = (struct icmp6hdr*)((char*)ipv6hdr + sizeof(*ipv6hdr));

	if (ipv6hdr->nexthdr == IPPROTO_ICMPV6 &&
				is_lan_ip6_addr(ipv6hdr->daddr) &&
				icmp6hdr->icmp6_type == ICMPV6_ECHO_REQUEST) {

		dpns_ip6_make_skb(skb);
		skb_push(skb, ETH_HLEN);
		skb_set_queue_mapping(skb, 1);
		skb->dev->netdev_ops->ndo_start_xmit(skb, skb->dev);

		return NF_STOLEN;
	}

	return NF_ACCEPT;
}

static unsigned int dpns_response_arp(struct sk_buff *skb)
{
	struct arphdr *arphdr;

	arphdr = get_arphdr(skb);

	if (arphdr->ar_op == htons(ARPOP_REQUEST) &&
			is_lan_ip4_addr(*(__be32*)((char*)arphdr+sizeof(*arphdr)
						+ 2 * ETH_ALEN + 4))) {
		dpns_arp_make_skb(skb);
		skb_push(skb, ETH_HLEN);
		skb_set_queue_mapping(skb, 1);
		skb->dev->netdev_ops->ndo_start_xmit(skb, skb->dev);

		return NF_STOLEN;
	}

	return NF_ACCEPT;
}

unsigned int dpns_fast_ping_hook(struct sk_buff *skb)
{
	unsigned int ret = NF_ACCEPT;

	if (skb->protocol == cpu_to_be16(ETH_P_8021Q)) {

		struct vlan_ethhdr *vlanhdr = (struct vlan_ethhdr*)(skb->data - ETH_HLEN);

		if (vlanhdr->h_vlan_encapsulated_proto == htons(ETH_P_IP)) {
			ret = dpns_response_icmp_v4(skb);
		} else if (vlanhdr->h_vlan_encapsulated_proto == htons(ETH_P_IPV6)) {
			ret = dpns_response_icmp_v6(skb);
		} else if (vlanhdr->h_vlan_encapsulated_proto == htons(ETH_P_ARP)) {
			ret = dpns_response_arp(skb);
		}
	} else {

		if (skb->protocol == htons(ETH_P_IP)) {
			ret = dpns_response_icmp_v4(skb);
		} else if (skb->protocol == htons(ETH_P_IPV6)) {
			ret = dpns_response_icmp_v6(skb);
		} else if (skb->protocol == htons(ETH_P_ARP)) {
			ret = dpns_response_arp(skb);
		}
	}

	return ret;
}
EXPORT_SYMBOL(dpns_fast_ping_hook);

static void dpns_fast_response_init_subnet_info(void)
{
        /* init lan subnet info */
	sprintf(sf_lan_addr[0].ifname, "br-lan");
	sprintf(sf_lan_addr[1].ifname, "br0");
	sprintf(sf_lan_addr[2].ifname, "br1");
	sprintf(sf_lan_addr[3].ifname, "br2");
	sprintf(sf_lan_addr[4].ifname, "br3");
	sprintf(sf_lan_addr[5].ifname, "br4");
}

static int dpns_fast_response_inetaddr_event(struct notifier_block *nb,
				unsigned long event, void *ptr)
{
	struct in_ifaddr *ifa = ptr;
	struct net_device *dev = ifa->ifa_dev->dev;
	__be32 ip;
	int i;

	ip = ifa->ifa_address;
	for (i = 0; i < 8; i++) {
		if (strncmp(sf_lan_addr[i].ifname, dev->name, IFNAMSIZ))
			continue;
		switch (event) {
		case NETDEV_UP:
			sf_lan_addr[i].v4.ip = ip;
			memcpy(sf_lan_addr[i].mac, dev->dev_addr, 6);
			return NOTIFY_OK;
		case NETDEV_DOWN:
			sf_lan_addr[i].v4.ip = 0;

			return NOTIFY_OK;
		}
	}

	return NOTIFY_DONE;
}

static int dpns_fast_response_inet6addr_event(struct notifier_block *nb,
				unsigned long event, void *ptr)
{
	struct inet6_ifaddr *ifa = (struct inet6_ifaddr *) ptr;
	struct net_device *dev = ifa->idev->dev;
	int i;


	for (i = 0; i < 8; i++) {
		if (strncmp(sf_lan_addr[i].ifname, dev->name, IFNAMSIZ))
			continue;
		switch (event) {
		case NETDEV_UP:
			if ((ifa->addr.s6_addr16[0] & htons(0xffc0)) == htons(0xfe80))
				memcpy(sf_lan_addr[i].v6.ip[0], ifa->addr.s6_addr,
							sizeof(struct in6_addr));
			else
				memcpy(sf_lan_addr[i].v6.ip[1], ifa->addr.s6_addr,
							sizeof(struct in6_addr));
			memcpy(sf_lan_addr[i].mac, dev->dev_addr, 6);
			return NOTIFY_OK;
		case NETDEV_DOWN:
			memset(sf_lan_addr[i].v6.ip[0], '\0',
						sizeof(struct in6_addr));
			memset(sf_lan_addr[i].v6.ip[1], '\0',
						sizeof(struct in6_addr));
			return NOTIFY_OK;
		}
	}

	return NOTIFY_DONE;
}

static struct notifier_block dpns_fast_response_inetaddr_notifier = {
	.notifier_call  = dpns_fast_response_inetaddr_event,
};

static struct notifier_block dpns_fast_response_inet6addr_notifier = {
	.notifier_call  = dpns_fast_response_inet6addr_event,
};

int fast_ping_probe(void)
{
	int ret;

	dpns_fast_response_init_subnet_info();

	ret = register_inetaddr_notifier(&dpns_fast_response_inetaddr_notifier);
	if (ret)
		goto err_ip4_notifier;

	ret = register_inet6addr_notifier(&dpns_fast_response_inet6addr_notifier);
	if (ret)
		goto err_ip6_notifier;

	return 0;

err_ip6_notifier:
	unregister_inet6addr_notifier(
		&dpns_fast_response_inet6addr_notifier);
err_ip4_notifier:
	unregister_inetaddr_notifier(
		&dpns_fast_response_inetaddr_notifier);
	return ret;
}
EXPORT_SYMBOL(fast_ping_probe);

void fast_ping_remove(void)
{
	unregister_inetaddr_notifier(
		&dpns_fast_response_inetaddr_notifier);
	unregister_inet6addr_notifier(
		&dpns_fast_response_inet6addr_notifier);
}
EXPORT_SYMBOL(fast_ping_remove);

MODULE_LICENSE("GPL v2");
