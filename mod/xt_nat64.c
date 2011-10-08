/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/netfilter/x_tables.h>
#include <linux/skbuff.h>
#include <net/ipv6.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/nf_conntrack_proto.h>
#include <net/netfilter/nf_conntrack_l3proto.h>
#include <net/netfilter/nf_conntrack_l4proto.h>
#include <net/netfilter/nf_nat.h>

#include "nf_nat64_bib.h"
#include "nf_nat64_tuple.h"
#include "xt_nat64.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Juan Antonio Osorio <jaosorior@gmail.com>");
MODULE_DESCRIPTION("Xtables: RFC 6146 \"NAT64\" implementation");
MODULE_ALIAS("ipt_nat64");
MODULE_ALIAS("ip6t_nat64");

#define IPV6_HDRLEN 40
static struct nf_conntrack_l3proto *l3proto_ip __read_mostly;
static struct nf_conntrack_l3proto *l3proto_ipv6 __read_mostly;

/*
 * IPv6 comparison function. It's use as a call from nat64_tg6 is to compare
 * the incoming packet's ip with the rule's ip, and so when the module is in
 * debugging mode it prints the rule's IP.
 */
static bool nat64_tg6_cmp(struct in6_addr * ip_a, struct in6_addr * ip_b,
		struct in6_addr * ip_mask)
{

	if (info->flags & XT_NAT64_IPV6_DST) {
		if (ipv6_masked_addr_cmp(ip_a, ip_mask, ip_b) != 0) 
			pr_debug("NAT64: IPv6 comparison returned true\n");
			return true;
	}

	pr_debug("NAT64: IPv6 comparison returned false\n");
	return false;
}

static bool nat64_determine_tuple(u_int8_t l3protocol, u_int8_t l4protocol, 
		struct sk_buff *skb, union nf_inet_ipaddr addr)
{
	const struct nf_conntrack_l4_proto *l4proto;
	struct nf_conntrack_tuple inner, target;
	int l3_hdrlen, l4_hdrlen;
	if (l3protocol == NFPROTO_IPV4)
		l3_hdrlen = ip_hdrlen(skb);
	else
		l3_hdrlen = IPV6_HDRLEN;

	inner =  memset(inner, 0, sizeof(*inner));

	l4proto = __nf_ct_l4proto_find(l3protocol, l4protocol);

	if (l4_protocol == IPPROTO_TCP)

	if (!nf_ct_get_tuple(skb, hdrlen + sizeof(struct icmphdr),
				(hdrlen +
				 sizeof(struct icmphdr) + inside->ip.ihl * 4),
				(u_int16_t)l3protocol, l4protocol,
				&inner, l3proto, l4proto))
		return 0;
}

/*
 * IPv4 entry function
 *
 */
static unsigned int nat64_tg4(struct sk_buff *skb, const struct xt_action_param *par)
{
	union nf_inet_addr;
	struct iphdr *iph = ip_hdr(skb);
	__u8 l4_protocol = iph->protocol;

	pr_debug("\n* ICNOMING IPV4 PACKET *\n");
	pr_debug("Drop it\n");

	return NF_DROP;
}

/*
 * IPv6 entry function
 *
 */
static unsigned int nat64_tg6(struct sk_buff *skb, const struct xt_action_param *par)
{
	union nf_inet_addr;
	const struct xt_nat64_tginfo *info = par->targinfo;
	struct ipv6hdr *iph = ipv6_hdr(skb);
	__u8 l4_protocol = iph->nexthdr;

	pr_debug("\n* ICNOMING IPV6 PACKET *\n");
	pr_debug("PKT SRC=%pI6 \n", &iph->saddr);
	pr_debug("PKT DST=%pI6 \n", &iph->daddr);
	pr_debug("RULE DST=%pI6 \n", &info->ip6dst.in6);
	pr_debug("RULE DST_MSK=%pI6 \n", &info->ip6dst_mask);

	if (!nat64_tg6_cmp(&info->ip6dst.in6, &info->ip6dst_mask.in6, &iph->saddr))
		return NF_DROP;

	if (l4_protocol & NAT64_IPV6_ALLWD_PROTOS)
		if(nat64_determine_tuple(NFPROTO_IPV6, l4_protocol, &skb, ))
			pr_debug("NAT64: Determining the tuple stage went OK.");
		else
			pr_debug("NAT64: Something went wrong in the determining the tuple"
					"stage.");

	return NF_DROP;

}

/*
 * General entry point. 
 *
 * Here the NAT64 implementation validates that the
 * incoming packet is IPv4 or IPv6. If it isn't, it silently drops the packet.
 * If it's one of those two, it calls it's respective function, since the IPv6
 * header is handled differently than an IPv4 header.
 */
static unsigned int nat64_tg(struct sk_buff *skb, const struct xt_action_param *par)
{
	if (par->family == NFPROTO_IPV4)
		return nat64_tg4(&sk, &par);
	else if (par->family == NFPROTO_IPV6)
		return nat64_tg6(&sk, &par);
	else
		return NF_DROP;
}

static int nat64_tg_check(const struct xt_tgchk_param *par)
{
	int ret;

	ret = nf_ct_l3proto_try_module_get(par->family);
	if (ret < 0)
		pr_info("cannot load support for proto=%u\n",
			par->family);
	return ret;
}

static struct xt_target nat64_tg_reg __read_mostly = {
	.name = "nat64",
	.revision = 0,
	.target = nat64_tg,
	.checkentry = nat64_tg_check,
	.family = NFPROTO_UNSPEC,
	.table = "mangle",
	.hooks = (1 << NF_INET_PRE_ROUTING),
	.targetsize = sizeof(struct xt_nat64_tginfo),
	.me = THIS_MODULE,
};

static int __init nat64_init(void)
{
	l3proto_ip = nf_ct_l3proto_find_get((u_int16_t)NFPROTO_IPV4);
	l3proto_ipv6 = nf_ct_l3proto_find_get((u_int16_t) NFPROTO_IPV6);
	return xt_register_targets(nat64_tg_reg, ARRAY_SIZE(nat64_tg_reg));
}

static void __exit nat64_exit(void)
{
	nf_ct_l3proto_put(l3proto_ip);
	nf_ct_l3proto_put(l3proto_ipv6);
	xt_unregister_targets(nat64_tg_reg, ARRAY_SIZE(nat64_tg_reg));
}

module_init(nat64_init);
module_exit(nat64_exit);
