#include <stdint.h>
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_arp.h>
#include <rte_icmp.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <rte_flow.h>
#include <rte_eth_ctrl.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include "recv_send.h"


#define RX_RING_SIZE 128

#define TX_RING_SIZE 512

#define NUM_MBUFS 8191

#define MBUF_CACHE_SIZE 250

#define BURST_SIZE 32

int receive_send_pkt(struct rte_mempool *mbuf_pool);
void  PRINT_MESSAGE(unsigned char *msg, int len);

static const struct rte_eth_conf port_conf_default = {
	.rxmode = { .max_rx_pkt_len = ETHER_MAX_LEN, },
	.txmode = { .offloads = DEV_TX_OFFLOAD_IPV4_CKSUM ,}
};

FILE *fp;

void  PRINT_MESSAGE(unsigned char *msg, int len)
{
	int  row_cnt,rows,rest_bytes,hex_cnt,ch_cnt,cnt,xi,ci;
	
	if (NULL == msg){
		printf("PRINT_MESSAGE(): NULL message ?\n");
		return;
	}
	
	/*if ((len*5) > 2048){ // 5 format bytes for one raw data byte
		printf("Too large[len(%d) > max(%d)] to print out!\n",len,2048);
		return;
	}*/
	
	rest_bytes = len % 16;
	rows = len / 16;
	ci = xi = 0;
	
	for(row_cnt=0; row_cnt<rows; row_cnt++){
		/*------------- print label for each row --------------*/
		printf("%04x:  ",(row_cnt+1)<<4);
		
		/*------------- print hex-part --------------*/
		for(hex_cnt=1; hex_cnt<=8; hex_cnt++){
		    if (hex_cnt < 8)
		        printf("%02x ",msg[xi++]); /* Must be unsigned, otherwise garbage displayed */
		    else
		        printf("%02x",msg[xi++]); /* Must be unsigned, otherwise garbage displayed */
		}
		
		/* delimiters space for each 8's Hex char */
		printf("  ");
		
		for(hex_cnt=9; hex_cnt<=16; hex_cnt++){
		    if (hex_cnt < 16)
		        printf("%02x ",msg[xi++]);
		    else
		        printf("%02x",msg[xi++]);
		}
		
		/* delimiters space bet. Hex and Character row */
		printf("    ");
		
		/*------------- print character-part --------------*/
		for(ch_cnt=1; ch_cnt<=16; ch_cnt++,ci++){
			if (msg[ci]>0x20 && msg[ci]<=0x7e){
		    	printf("%c",msg[ci]);
		    }
		    else{
		    	printf(".");
			}
		}
		printf("\n");
	} //for
	
	/*================ print the rest bytes(hex & char) ==================*/
	if (rest_bytes == 0) {
		printf("\n");
		return;
	}
	
	/*------------- print label for last row --------------*/
	printf("%04x:  ",(row_cnt+1)<<4);
		
	/*------------- print hex-part(rest) --------------*/
	if (rest_bytes < 8){
	    for(hex_cnt=1; hex_cnt<=rest_bytes; hex_cnt++){
	        printf("%02x ",msg[xi++]);
	    }
	
	    /* fill in the space for 16's Hex-part alignment */
	    for(cnt=rest_bytes+1; cnt<=8; cnt++){ /* from rest_bytes+1 to 8 */
	        if (cnt < 8)
	            printf("   ");
	        else
	            printf("  ");
	    }
	
	    /* delimiters bet. hex and char */
	    printf("  ");
	
	    for(cnt=9; cnt<=16; cnt++){
	        if (cnt < 16)
	            printf("   ");
	        else
	            printf("  ");
	    }
	    printf("    ");
	}
	else if (rest_bytes == 8){
	    for(hex_cnt=1; hex_cnt<=rest_bytes; hex_cnt++){
	        if (hex_cnt < 8)
	            printf("%02x ",msg[xi++]);
	        else
	            printf("%02x",msg[xi++]);
	    }
	    printf("  ");
	
	    for(cnt=9; cnt<=16; cnt++){
	        if (cnt < 16)
	            printf("   ");
	        else
	            printf("  ");
	    }
	    printf("    ");
	}
	else{ /* rest_bytes > 8 */
	    for(hex_cnt=1; hex_cnt<=8; hex_cnt++){
	        if (hex_cnt < 8)
	            printf("%02x ",msg[xi++]);
	        else
	            printf("%02x",msg[xi++]);
	    }
	
	    /* delimiters space for each 8's Hex char */
	    printf("  ");
	
	    for(hex_cnt=9; hex_cnt<=rest_bytes; hex_cnt++){ /* 9 - rest_bytes */
	        if (hex_cnt < 16)
	            printf("%02x ",msg[xi++]);
	        else
	            printf("%02x",msg[xi++]);
	    }
	
	    for(cnt=rest_bytes+1; cnt<=16; cnt++){
	        if (cnt < 16)
	            printf("   ");
	        else
	            printf("  ");
	    }
	    /* delimiters space bet. Hex and Character row */
	    printf("    ");
	} /* else */
	
	/*------------- print character-part --------------*/
	for(ch_cnt=1; ch_cnt<=rest_bytes; ch_cnt++,ci++){
		if (msg[ci]>0x20 && msg[ci]<=0x7e){
	        printf("%c",msg[ci]);
	    }
	    else
	        printf(".");
	}
	printf("\n");
}

/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2017 Mellanox Technologies, Ltd
 */

#define MAX_PATTERN_NUM		4

struct rte_flow *
generate_flow(uint16_t port_id, uint16_t rx_q, struct rte_flow_error *error);

struct rte_flow *
generate_flow(uint16_t port_id, uint16_t rx_q, struct rte_flow_error *error)
{
	struct rte_eth_flex_filter filter;
	memset(&filter,0,sizeof(struct rte_eth_flex_filter));
	filter.len = 24;
	filter.priority = 1;
	filter.queue = rx_q;
	uint8_t bytes[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
						0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00};//,
						//0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01};
	uint8_t mask[] = {0x0c};
	memcpy(filter.bytes,bytes,16);
	memcpy(filter.mask,mask,1);
	rte_eth_dev_filter_ctrl(port_id,RTE_ETH_FILTER_FLEXIBLE,RTE_ETH_FILTER_ADD,&filter);

#if 0
	struct rte_flow_attr attr;
	struct rte_flow_item pattern[MAX_PATTERN_NUM];
	struct rte_flow_action action[MAX_PATTERN_NUM];
	struct rte_flow *flow = NULL;
	struct rte_flow_action_queue queue = { .index = rx_q };
	struct rte_flow_item_eth eth_spec;
	struct rte_flow_item_eth eth_mask;
	struct rte_flow_item_ipv4 ip_spec;
	struct rte_flow_item_ipv4 ip_mask;
	struct rte_flow_item_raw raw_spec;
	struct rte_flow_item_raw raw_mask;
	uint8_t spec[12] 	= {0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01};
	uint8_t mask[12] 	= {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	int res;

	memset(pattern, 0, sizeof(pattern));
	memset(action, 0, sizeof(action));

	/*
	 * set the rule attribute.
	 * in this case only ingress packets will be checked.
	 */
	memset(&attr, 0, sizeof(struct rte_flow_attr));
	attr.ingress = 1;

	/*
	 * create the action sequence.
	 * one action only,  move packet to queue
	 */

	action[0].type = RTE_FLOW_ACTION_TYPE_QUEUE;
	action[0].conf = &queue;
	action[1].type = RTE_FLOW_ACTION_TYPE_END;

	/*
	 * set the first level of the pattern (eth).
	 * since in this example we just want to get the
	 * ipv4 we set this level to allow all.
	 */
	memset(&eth_spec, 0, sizeof(struct rte_flow_item_eth));
	memset(&eth_mask, 0, sizeof(struct rte_flow_item_eth));
	eth_spec.type = htons(0x0806);
	eth_mask.type = 0xffff;
	pattern[0].type = RTE_FLOW_ITEM_TYPE_ETH;
	pattern[0].spec = &eth_spec;
	pattern[0].mask = &eth_mask;
	/* the final level must be always type end */
	pattern[1].type = RTE_FLOW_ITEM_TYPE_END;

	res = rte_flow_validate(port_id, &attr, pattern, action, error);
	if (!res)
		flow = rte_flow_create(port_id, &attr, pattern, action, error);

	memset(pattern,0,sizeof(pattern));
	memset(action,0,sizeof(action));

	/*
	 * set the rule attribute.
	 * in this case only ingress packets will be checked.
	 */
	memset(&attr, 0, sizeof(struct rte_flow_attr));
	attr.ingress = 1;

	/*
	 * create the action sequence.
	 * one action only,  move packet to queue
	 */

	action[0].type = RTE_FLOW_ACTION_TYPE_QUEUE;
	action[0].conf = &queue;
	action[1].type = RTE_FLOW_ACTION_TYPE_END;

	/*
	 * set the first level of the pattern (eth).
	 * since in this example we just want to get the
	 * ipv4 we set this level to allow all.
	 */
	memset(&eth_spec,0,sizeof(struct rte_flow_item_eth));
	memset(&eth_mask,0,sizeof(struct rte_flow_item_eth));
	/*memcpy(eth_spec.dst.addr_bytes,mac_addr[port_id],ETH_ALEN);
	for(int i=0; i<ETH_ALEN; i++)
		eth_mask.dst.addr_bytes[i] = 0xff;*/
	eth_spec.type = 0;
	eth_mask.type = 0;
	pattern[0].type = RTE_FLOW_ITEM_TYPE_ETH;
	pattern[0].spec = &eth_spec;
	pattern[0].mask = &eth_mask;

	/*
	 * setting the third level of the pattern (ip).
	 * in this example this is the level we care about
	 * so we set it according to the parameters.
	 */
	memset(&ip_spec, 0, sizeof(struct rte_flow_item_ipv4));
	memset(&ip_mask, 0, sizeof(struct rte_flow_item_ipv4));
	ip_spec.hdr.next_proto_id = 0x1;
	ip_mask.hdr.next_proto_id = 0xff;
	
	pattern[1].type = RTE_FLOW_ITEM_TYPE_IPV4;
	pattern[1].spec = &ip_spec;
	pattern[1].mask = &ip_mask;

	/* the final level must be always type end */
	pattern[2].type = RTE_FLOW_ITEM_TYPE_END;

	res = rte_flow_validate(port_id, &attr, pattern, action, error);
	if (!res)
		flow = rte_flow_create(port_id, &attr, pattern, action, error);

	return flow;
#endif
}

static inline int
port_init(uint16_t port, struct rte_mempool *mbuf_pool)
{
	struct rte_eth_conf port_conf = port_conf_default;
	struct rte_eth_dev_info dev_info;
	const uint16_t rx_rings = 2, tx_rings = 2;
	int retval;
	uint16_t q;

	if (!rte_eth_dev_is_valid_port(port))
		return -1;
	rte_eth_dev_info_get(port, &dev_info);
	if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
		port_conf.txmode.offloads |=
			DEV_TX_OFFLOAD_MBUF_FAST_FREE;

	retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
	if (retval != 0)
		return retval;

	/* Allocate and set up 1 RX queue per Ethernet port. */
	for (q = 0; q < rx_rings; q++) {
		retval = rte_eth_rx_queue_setup(port, q, RX_RING_SIZE,
				rte_eth_dev_socket_id(port), NULL, mbuf_pool);
		if (retval < 0)
			return retval;
	}

	/* Allocate and set up 1 TX queue per Ethernet port. */
	for (q = 0; q < tx_rings; q++) {
		retval = rte_eth_tx_queue_setup(port, q, TX_RING_SIZE,
				rte_eth_dev_socket_id(port), NULL);
		if (retval < 0)
			return retval;
	}

	/* Start the Ethernet port. */
	retval = rte_eth_dev_start(port);
	if (retval < 0)
		return retval;
	//rte_eth_promiscuous_enable(port);
	return 0;
}

int receive_send_pkt(struct rte_mempool *mbuf_pool)
{
	uint64_t total_tx;
	uint64_t recv_size = 0;
	struct rte_mbuf *single_pkt;
	unsigned char mac_addr[6];// = {0x76,0xfb,0xc5,0x78,0x6e,0xd5};
	rte_eth_macaddr_get(0,(struct ether_addr *)mac_addr);
	printf("mac = %x:%x:%x:%x:%x:%x\n", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
	uint32_t ip_addr = htonl(0xc0a80266);
	uint64_t cur_tsc;
	uint64_t prev_tsc = 0;
	float cur_time = 0;
	uint64_t cur_clock = 0;

	struct ether_hdr *eth_hdr;
	struct icmp_hdr *icmphdr;
	struct udp_hdr udphdr;
	struct arp_hdr *arphdr;
	struct rte_mbuf *pkt[BURST_SIZE];

	for(;;) {
		total_tx = 0;
		/*for(i=0;i<BURST_SIZE;i++) {
			pkt[i] = rte_pktmbuf_alloc(mbuf_pool);
			if (pkt[i] != 0)
				total_addr++;
			//printf("pkt addr = %x\n", pkt[i]);
		}*/
		//printf("in core %u\n", rte_lcore_id());
		/*uint16_t port;
		RTE_ETH_FOREACH_DEV(port) {
			printf("port id = %u\n", port);
		}*/
		uint16_t nb_rx = rte_eth_rx_burst(0, 0,pkt,BURST_SIZE);
		//printf("nb_rx = %x\n", nb_rx);
		//printf("total addr = %lu\n", total_addr);
		if(nb_rx == 0) {
			//for(i=0;i<BURST_SIZE;i++)
			//rte_pktmbuf_free(pkt[i]);
			continue;
		}
		//printf("nb_rx = %x\n", nb_rx);
		//fp = fopen("test.txt","a");
		//fprintf(fp, "nb rx = %x\n", nb_rx);
		for(int i=0;i<nb_rx;i++) {
			single_pkt = pkt[i];
			rte_prefetch0(rte_pktmbuf_mtod(single_pkt, void *));
			//printf("pkt->pkt_len = %x data_len  = %x\n", pkt[i]->pkt_len, pkt[i]->data_len);
			//fprintf(fp, "%d %d %x %x %x %x %c", i, nb_rx, pkt[i]->pkt_len, pkt[i]->data_off, pkt[i]->buf_addr, rte_pktmbuf_mtod(pkt[i],struct ether_hdr*), '\n');
			//printf("i = %d nb_rx = %d\n", i, nb_rx);
			//printf("mbuf addr = %x\n", (uint32_t *)(pkt[i]->buf_addr));
			eth_hdr = rte_pktmbuf_mtod(single_pkt,struct ether_hdr*);
			//printf("ether type = %x\n", eth_hdr->ether_type);
			if (eth_hdr->ether_type == htons(0x0806)) {
				puts("recv arp packet on queue 0");
				memcpy(eth_hdr->d_addr.addr_bytes,eth_hdr->s_addr.addr_bytes,6);
				memcpy(eth_hdr->s_addr.addr_bytes,mac_addr,6);
				arphdr = (struct arp_hdr *)(rte_pktmbuf_mtod(single_pkt, unsigned char *) + sizeof(struct ether_hdr));
				if (arphdr->arp_op == htons(0x0001) && arphdr->arp_data.arp_tip == ip_addr) {
					memcpy(arphdr->arp_data.arp_tha.addr_bytes,arphdr->arp_data.arp_sha.addr_bytes,6);
					memcpy(arphdr->arp_data.arp_sha.addr_bytes,mac_addr,6);
					arphdr->arp_data.arp_tip = arphdr->arp_data.arp_sip;
					arphdr->arp_data.arp_sip = ip_addr;
					arphdr->arp_op = htons(0x0002);
				}
				pkt[total_tx++] = single_pkt;
				/*rte_pktmbuf_free(single_pkt);
				continue;*/
			}
			else {
				memcpy(eth_hdr->d_addr.addr_bytes,eth_hdr->s_addr.addr_bytes,6);
				memcpy(eth_hdr->s_addr.addr_bytes,mac_addr,6);
				struct ipv4_hdr *ip_hdr = (struct ipv4_hdr *)(rte_pktmbuf_mtod(single_pkt, unsigned char *) + sizeof(struct ether_hdr));
				ip_hdr->dst_addr = ip_hdr->src_addr;
				ip_hdr->src_addr = ip_addr;
				single_pkt->ol_flags |= PKT_TX_IPV4 | PKT_TX_IP_CKSUM;
				single_pkt->l2_len = sizeof(struct ether_hdr);
				single_pkt->l3_len = sizeof(struct ipv4_hdr);
				ip_hdr->hdr_checksum = 0;
				//printf("%lu\n", rte_rdtsc());
				switch (ip_hdr->next_proto_id) {
					 case IPV4_UDP:
					 	  //udphdr = (struct udp_hdr *)(rte_pktmbuf_mtod(pkt[i], unsigned char *) + sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr));
						  recv_size += single_pkt->data_len;
						  cur_tsc = rte_rdtsc();
						  if (likely(prev_tsc != 0)) {
						  		cur_clock = rte_get_tsc_hz();
						  		cur_time += (float)(cur_tsc - prev_tsc) / (float)cur_clock;
						  //printf("diff_tsc = %lu, cur_clock = %lu\n", cur_tsc - prev_tsc, cur_clock);
						  //printf("%.9f\n", cur_time);
						  		if (unlikely(cur_time >= 1)) {
						  	   		printf("tx rate = %lu Mb/s\n", recv_size/131072);
						  	   		cur_time = 0;
						  	   		recv_size = 0;
						  	   	}
						  }
						  rte_pktmbuf_free(single_pkt);
						  prev_tsc = cur_tsc;
						  break;
					default:
						rte_pktmbuf_free(single_pkt);
						puts("recv not udp packet on queue 0");
						;
				}
			}
		}
		//printf("pkt addr = %x, nb_rx = %d\n", pkt, nb_rx);
		//fclose(fp);
		if (total_tx > 0) {
			uint16_t nb_tx = rte_eth_tx_burst(0, 0,pkt, total_tx);
		//fprintf(fp, "nb_tx = %x\n", nb_tx);
		/*for(i=0;i<BURST_SIZE;i++)
				rte_pktmbuf_free(pkt[i]);*/
			if (unlikely(nb_tx < total_tx)) {
				uint16_t buf;
				for(buf = nb_tx; buf < total_tx; buf++)
					rte_pktmbuf_free(pkt[buf]);
			}
		}
		//printf("<%d\n", __LINE__);
	}
	return 0;

}

int recv_arp(struct rte_mempool *mbuf_pool)
{
	uint64_t total_tx;
	uint64_t recv_size = 0;
	struct rte_mbuf *single_pkt;
	unsigned char mac_addr[6];// = {0x76,0xfb,0xc5,0x78,0x6e,0xd5};
	rte_eth_macaddr_get(0,(struct ether_addr *)mac_addr);
	printf("mac = %x:%x:%x:%x:%x:%x\n", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

	struct ether_hdr *eth_hdr;
	struct arp_hdr *arphdr;
	struct icmp_hdr *icmphdr;
	struct rte_mbuf *pkt[BURST_SIZE];
	uint32_t ip_addr = htonl(0xc0a80266);

	for(;;) {
		total_tx = 0;
		uint16_t nb_rx = rte_eth_rx_burst(0, 1,pkt,BURST_SIZE);
		if (nb_rx == 0)
			continue;
		for(int i=0;i<nb_rx;i++) {
			single_pkt = pkt[i];
			rte_prefetch0(rte_pktmbuf_mtod(single_pkt, void *));
			eth_hdr = rte_pktmbuf_mtod(single_pkt,struct ether_hdr*);
			if (eth_hdr->ether_type == htons(0x0806)) {
				puts("recv arp packet on queue 1");
				memcpy(eth_hdr->d_addr.addr_bytes,eth_hdr->s_addr.addr_bytes,6);
				memcpy(eth_hdr->s_addr.addr_bytes,mac_addr,6);
				arphdr = (struct arp_hdr *)(rte_pktmbuf_mtod(single_pkt, unsigned char *) + sizeof(struct ether_hdr));
				if (arphdr->arp_op == htons(0x0001) && arphdr->arp_data.arp_tip == ip_addr) {
					memcpy(arphdr->arp_data.arp_tha.addr_bytes,arphdr->arp_data.arp_sha.addr_bytes,6);
					memcpy(arphdr->arp_data.arp_sha.addr_bytes,mac_addr,6);
					arphdr->arp_data.arp_tip = arphdr->arp_data.arp_sip;
					arphdr->arp_data.arp_sip = ip_addr;
					arphdr->arp_op = htons(0x0002);
				}
				pkt[total_tx++] = single_pkt;
			}
			else {
				memcpy(eth_hdr->d_addr.addr_bytes,eth_hdr->s_addr.addr_bytes,6);
				memcpy(eth_hdr->s_addr.addr_bytes,mac_addr,6);
				struct ipv4_hdr *ip_hdr = (struct ipv4_hdr *)(rte_pktmbuf_mtod(single_pkt, unsigned char *) + sizeof(struct ether_hdr));
				ip_hdr->dst_addr = ip_hdr->src_addr;
				ip_hdr->src_addr = ip_addr;
				single_pkt->ol_flags |= PKT_TX_IPV4 | PKT_TX_IP_CKSUM;
				single_pkt->l2_len = sizeof(struct ether_hdr);
				single_pkt->l3_len = sizeof(struct ipv4_hdr);
				ip_hdr->hdr_checksum = 0;
				switch (ip_hdr->next_proto_id) {
					 case IPV4_ICMP:
					 	puts("recv icmp packet");
					 	  icmphdr = (struct icmp_hdr *)(rte_pktmbuf_mtod(single_pkt, unsigned char *) + sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr));
						  icmphdr->icmp_type = 0;
						  uint32_t cksum = ~icmphdr->icmp_cksum & 0xffff;
						  cksum += ~htons(8 << 8) & 0xffff;
						  cksum += htons(0 << 8);
		  				  cksum = (cksum & 0xffff) + (cksum >> 16);
						  cksum = (cksum & 0xffff) + (cksum >> 16);
						  icmphdr->icmp_cksum = ~cksum;
						  pkt[total_tx++] = single_pkt;
						 break;
					default:
						rte_pktmbuf_free(single_pkt);
						puts("recv other packet at queue 1");
						;
				}
			}
		}
		if (total_tx > 0) {
			uint16_t nb_tx = rte_eth_tx_burst(0, 1,pkt, total_tx);
			if (unlikely(nb_tx < total_tx)) {
				uint16_t buf;
				for(buf = nb_tx; buf < total_tx; buf++)
					rte_pktmbuf_free(pkt[buf]);
			}
		}
	}
	return 0;

}

int main(int argc, char *argv[])
{
	 struct rte_mempool *mbuf_pool;
	 uint16_t portid;
	 struct rte_flow_error error;
	struct rte_flow *flow;

	 int ret = rte_eal_init(argc, argv);
	 if (ret < 0)
		  rte_exit(EXIT_FAILURE, "initlize fail!");

	argc -= ret;
	argv += ret;

	if (rte_lcore_count() != 3)
		rte_exit(EXIT_FAILURE, "We only need 3 cores\n");

	/* Creates a new mempool in memory to hold the mbufs. */
	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS,
		MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

	/* Initialize all ports. */
	RTE_ETH_FOREACH_DEV(portid) {
		if (port_init(portid,mbuf_pool) != 0)
			rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu8 "\n",portid);
	}
	flow = generate_flow(0,1,&error);
	
	if (!flow) {
		printf("Flow can't be created %d message: %s\n",
			error.type,
			error.message ? error.message : "(no stated reason)");
		rte_exit(EXIT_FAILURE, "error in creating flow");
	}

	//unsigned lcore_id;
	//RTE_LCORE_FOREACH_SLAVE(lcore_id) {
        rte_eal_remote_launch((void *)recv_arp, mbuf_pool, 1);
        rte_eal_remote_launch((void *)receive_send_pkt, mbuf_pool, 2);
    //}
	rte_eal_mp_wait_lcore();
	
	return 0;
}