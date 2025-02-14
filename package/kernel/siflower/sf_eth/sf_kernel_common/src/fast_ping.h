#include <uapi/linux/if.h>
#include <linux/skbuff.h>
#include <linux/if_arp.h>
#include <linux/ip.h>
#include <uapi/linux/in6.h>
#include <uapi/linux/ip.h>
#include <uapi/linux/ipv6.h>
#include <uapi/linux/if_arp.h>
#include <uapi/linux/if_ether.h>
#include <uapi/linux/in.h>
#include <net/if_inet6.h>
#include <net/ip.h>

struct dpns_fast_response_lan_addr_info {
	struct {
		__be32	ip;
	} v4;
	struct {
		u8	ip[2][16];
	} v6;
	u8	mac[6];
	char	ifname[IFNAMSIZ];
};

struct arp_data {
	u8 	sender_mac[6];
	u8	sender_ip[4];
	u8	target_mac[6];
	u8	target_ip[4];
};

static inline void swap_mac_addr(u8* addr1, u8* addr2)
{
	int i;

	for (i = 0; i < ETH_ALEN; i++) {
		unsigned char tmp;

		tmp = addr1[i];
		addr1[i] = addr2[i];
		addr2[i] = tmp;
	}
}

static inline struct iphdr *get_iphdr(struct sk_buff* skb)
{
	struct iphdr *iphdr;

	if (skb->protocol == htons(ETH_P_8021Q))
		iphdr = (struct iphdr*)(skb->data + 4);
	else
		iphdr = (struct iphdr*)(skb->data);

	return iphdr;
}

static inline struct ipv6hdr *get_ipv6hdr(struct sk_buff* skb)
{
	struct ipv6hdr *ipv6hdr;

	if (skb->protocol == htons(ETH_P_8021Q))
		ipv6hdr = (struct ipv6hdr*)(skb->data + 4);
	else
		ipv6hdr = (struct ipv6hdr*)skb->data;

	return ipv6hdr;
}

static inline struct arphdr *get_arphdr(struct sk_buff* skb)
{
	struct arphdr *arphdr;

	if(skb->protocol == htons(ETH_P_8021Q))
		arphdr = (struct arphdr*)(skb->data + 4);
	else
		arphdr = (struct arphdr*)skb->data;

	return arphdr;
}

static inline  __u32 from32to16(__u32 x)
{
        /* add up 16-bit and 16-bit for 16+c bit */
        x = (x & 0xffff) + (x >> 16);
        /* add up carry.. */
        x = (x & 0xffff) + (x >> 16);
        return x;
}

static inline void dump_eth(struct sk_buff* skb)
{
	struct ethhdr *ethhdr;

	ethhdr = (struct ethhdr *)(skb->data - ETH_HLEN);

	printk("saddr :%pM,  dadddr :%pM", &ethhdr->h_source, &ethhdr->h_dest);
}

static inline void dump_ip4(struct iphdr* iphdr)
{
	printk("s_ip  :%pI4, d_ip   :%pI4, csm:  ", &iphdr->saddr, &iphdr->daddr);
}

static inline void dump_ip6(struct ipv6hdr* ipv6hdr)
{
	printk("s_ip  :%pI6, d_ip   :%pI6", &ipv6hdr->saddr, &ipv6hdr->daddr);
}

static inline void dump_arp(struct arphdr* arphdr)
{
	struct arp_data* arpdata = (struct arp_data*)((char*)arphdr
					+ sizeof(*arphdr));

	printk("类型：%x", ntohs(arphdr->ar_op));
	printk("sip:%pI4 dip:%pI4 smac:%pM dmac:%pM", arpdata->sender_ip,
		arpdata->target_ip, arpdata->sender_mac, arpdata->target_mac);
}

