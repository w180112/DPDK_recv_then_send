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
#include <pthread.h>
#include <string.h>
#include <stdio.h>


#define RX_RING_SIZE 128

#define TX_RING_SIZE 512

#define NUM_MBUFS 8191

#define MBUF_CACHE_SIZE 250

#define BURST_SIZE 32

int receive_send_pkt(struct rte_mempool *mbuf_pool);
void  PRINT_MESSAGE(unsigned char *msg, int len);

static const struct rte_eth_conf port_conf_default = {
	.rxmode = { .max_rx_pkt_len = ETHER_MAX_LEN, }

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


static inline int
port_init(uint16_t port, struct rte_mempool *mbuf_pool)
{
	struct rte_eth_conf port_conf = port_conf_default;
	struct rte_eth_dev_info dev_info;
	const uint16_t rx_rings = 1, tx_rings = 1;
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
	//uint64_t total_addr;
	unsigned char mac_addr[6];// = {0x76,0xfb,0xc5,0x78,0x6e,0xd5};
	rte_eth_macaddr_get(0,(struct ether_addr *)mac_addr);
	printf("mac = %x:%x:%x:%x:%x:%x\n", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
	uint32_t ip_addr = htonl(0xc0a80166);
	for(;;) {
		struct rte_mbuf * pkt[BURST_SIZE];
		int i;
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
		printf("nb_rx = %x\n", nb_rx);
		struct ether_hdr * eth_hdr;
		fp = fopen("test.txt","a");
		//fprintf(fp, "nb rx = %x\n", nb_rx);
		for(i=0;i<nb_rx;i++) {
			//printf("pkt->pkt_len = %x data_len  = %x\n", pkt[i]->pkt_len, pkt[i]->data_len);
			//fprintf(fp, "%d %d %x %x %x %x %c", i, nb_rx, pkt[i]->pkt_len, pkt[i]->data_off, pkt[i]->buf_addr, rte_pktmbuf_mtod(pkt[i],struct ether_hdr*), '\n');
			//printf("i = %d nb_rx = %d\n", i, nb_rx);
			//printf("mbuf addr = %x\n", (uint32_t *)(pkt[i]->buf_addr));
			eth_hdr = rte_pktmbuf_mtod(pkt[i],struct ether_hdr*);
			//printf("ether type = %x\n", eth_hdr->ether_type);
			if (eth_hdr->ether_type == htons(0x0806)) {
				//PRINT_MESSAGE((char *)eth_hdr,pkt[i]->data_len);
				memcpy(eth_hdr->d_addr.addr_bytes,eth_hdr->s_addr.addr_bytes,6);
				memcpy(eth_hdr->s_addr.addr_bytes,mac_addr,6);
				struct arp_hdr *arphdr = (struct arp_hdr *)(rte_pktmbuf_mtod(pkt[i], unsigned char *) + sizeof(struct ether_hdr));
				//fprintf(fp, "arp src ip = %x op code = %x\n", arphdr->arp_data.arp_sip, arphdr->arp_op);
				if (arphdr->arp_op == htons(0x0001) && arphdr->arp_data.arp_tip == ip_addr) {
					//printf("<%d\n", __LINE__);
					memcpy(arphdr->arp_data.arp_tha.addr_bytes,arphdr->arp_data.arp_sha.addr_bytes,6);
					memcpy(arphdr->arp_data.arp_sha.addr_bytes,mac_addr,6);
					arphdr->arp_data.arp_tip = arphdr->arp_data.arp_sip;
					arphdr->arp_data.arp_sip = ip_addr;
					arphdr->arp_op = htons(0x0002);
				}
				//fprintf(fp, "src mac = %x:%x:%x:%x:%x:%x dst mac = %x:%x:%x:%x:%x:%x\n", arphdr->arp_data.arp_sha.addr_bytes[0], arphdr->arp_data.arp_sha.addr_bytes[1], arphdr->arp_data.arp_sha.addr_bytes[2], arphdr->arp_data.arp_sha.addr_bytes[3], arphdr->arp_data.arp_sha.addr_bytes[4], arphdr->arp_data.arp_sha.addr_bytes[5], arphdr->arp_data.arp_tha.addr_bytes[0], arphdr->arp_data.arp_tha.addr_bytes[1], arphdr->arp_data.arp_tha.addr_bytes[2], arphdr->arp_data.arp_tha.addr_bytes[3], arphdr->arp_data.arp_tha.addr_bytes[4], arphdr->arp_data.arp_tha.addr_bytes[5]);
			}
			/*printf("recv packet from: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
				   " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 " : ",
				eth_hdr->s_addr.addr_bytes[0],eth_hdr->s_addr.addr_bytes[1],
				eth_hdr->s_addr.addr_bytes[2],eth_hdr->s_addr.addr_bytes[3],
				eth_hdr->s_addr.addr_bytes[4],eth_hdr->s_addr.addr_bytes[5]);*/
			else {
				//unsigned char dst_mac[6] = {0x7a,0x1f,0x95,0x13,0x8d,0x38};
				//unsigned char src_mac[6] = {0xbe,0xc2,0x46,0xe9,0x1a,0x24};
				memcpy(eth_hdr->d_addr.addr_bytes,eth_hdr->s_addr.addr_bytes,6);
				memcpy(eth_hdr->s_addr.addr_bytes,mac_addr,6);
				struct ipv4_hdr *ip_hdr = (struct ipv4_hdr *)(rte_pktmbuf_mtod(pkt[i], unsigned char *) + sizeof(struct ether_hdr));
				ip_hdr->dst_addr = ip_hdr->src_addr;
				ip_hdr->src_addr = ip_addr;
				struct icmp_hdr *icmphdr;
				pkt[i]->ol_flags |= PKT_TX_IPV4 | PKT_TX_IP_CKSUM;
				pkt[i]->l2_len = sizeof(struct ether_hdr);
				pkt[i]->l3_len = sizeof(struct ipv4_hdr);
				ip_hdr->hdr_checksum = 0;
				switch (ip_hdr->next_proto_id) {
					 case 1:
					 	  icmphdr = (struct icmp_hdr *)(rte_pktmbuf_mtod(pkt[i], unsigned char *) + sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr));
						  icmphdr->icmp_type = 0;
						  uint32_t cksum = ~icmphdr->icmp_cksum & 0xffff;
						  cksum += ~htons(8 << 8) & 0xffff;
						  cksum += htons(0 << 8);
		  				  cksum = (cksum & 0xffff) + (cksum >> 16);
						  cksum = (cksum & 0xffff) + (cksum >> 16);
						  icmphdr->icmp_cksum = ~cksum;
						 break;
					default:
						;
				}
			}
		}
		//printf("pkt addr = %x, nb_rx = %d\n", pkt, nb_rx);
		fclose(fp);
		uint16_t nb_tx = rte_eth_tx_burst(0, 0,pkt, nb_rx);
		//fprintf(fp, "nb_tx = %x\n", nb_tx);
		/*for(i=0;i<BURST_SIZE;i++)
				rte_pktmbuf_free(pkt[i]);*/
		if (unlikely(nb_tx < nb_rx)) {
				uint16_t buf;
				for (buf = nb_tx; buf < nb_rx; buf++)
					rte_pktmbuf_free(pkt[buf]);
		}
		//printf("<%d\n", __LINE__);
	}
	return 0;

}

int main(int argc, char *argv[])
{
	 struct rte_mempool *mbuf_pool;
	 uint16_t portid;

	 int ret = rte_eal_init(argc, argv);
	 if (ret < 0)
		  rte_exit(EXIT_FAILURE, "initlize fail!");

	argc -= ret;
	argv += ret;

	if (rte_lcore_count() < 2) {
		 puts("Need 2 cores at least");
		 return 0;
	}


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

	unsigned lcore_id;
	RTE_LCORE_FOREACH_SLAVE(lcore_id) {
        rte_eal_remote_launch((void *)receive_send_pkt, mbuf_pool, lcore_id);
    }
	rte_eal_mp_wait_lcore();
	
	return 0;
}