/*
 * Copyright (c) 2014, 2015 Nicira, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <config.h>

#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <sched.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/icmp6.h>
#include <netinet/ip6.h>
#include "dp-packet.h"
#include "packets.h"
#include "flow.h"
#include "unaligned.h"
#include "util.h"

#include "probe-generator.h"
#include "openvswitch/vlog.h"

#include "ovs-thread.h"


VLOG_DEFINE_THIS_MODULE(probe_genrator);
static struct vlog_rate_limit rl = VLOG_RATE_LIMIT_INIT(5, 20);

#define PROBE_GEN_INTERVAL 5
#define PROBE_GEN_SRC_MAC "11:22:33:44:55:66"
#define PROBE_GEN_DST_MAC "22:33:44:55:66:77"
#define PROBE_GEN_SRC_IP "9.9.9.9"
#define PROBE_GEN_DST_IP "9.9.9.10"
#define PROBE_GEN_DATA 0xFEED  // read this using the load-avg or cpu-util functions
#define PROBE_GEN_OUTPORT 0x1

// TODO: Do we need this?
static struct ovs_mutex probe_gen_mutex = OVS_MUTEX_INITIALIZER;

static struct dp_packet *probe_pkt = NULL;

OVS_PACKED(
        struct probe_header {
            ovs_be32 data;
        }
);

static struct dp_packet *compose_probe_pkt(const struct eth_addr src_mac, const struct eth_addr dst_mac,
                                           uint32_t src_ip, uint32_t dst_ip, uint32_t data) {
    struct dp_packet *p = dp_packet_new(0);

    eth_compose(p, dst_mac, src_mac, ETH_TYPE_IP, 0);

    struct ip_header *ip;
    ip = dp_packet_put_zeros(p, sizeof *ip);
    ip->ip_ihl_ver = IP_IHL_VER(5, 4);
    ip->ip_tos = 0;
    ip->ip_ttl = 64;
    ip->ip_proto = 0xf0;
    put_16aligned_be32(&ip->ip_src, htonl(src_ip));
    put_16aligned_be32(&ip->ip_dst, htonl(dst_ip));

//    dp_packet_set_l4(p, dp_packet_tail(p));
//    struct udp_header *udp;
//    size_t l4_len = sizeof *udp;
//    udp = dp_packet_put_zeros(p, l4_len);
//    udp->udp_src = htons(0x1234);
//    udp->udp_dst = htons(0x2345);

    dp_packet_set_l4(p, dp_packet_tail(p));
    struct probe_header *prb;
    size_t prb_len = sizeof *prb;
    prb = dp_packet_put_zeros(p, prb_len);
    prb->data = data;

    ip = dp_packet_l3(p);
    ip->ip_tot_len = htons(p->l4_ofs - p->l3_ofs + prb_len);
    ip->ip_csum = csum(ip, sizeof *ip);

    return p;
}

static uint32_t thresh_val = -1;

static void
odp_execute_send_probe(void *dp, odp_execute_gp gp_execute_action) {
    /*uint8_t thresh = XXX;*/
    uint16_t output = PROBE_GEN_OUTPORT;
    struct eth_addr src_mac;
    str_to_mac(PROBE_GEN_SRC_MAC, &src_mac);
    struct eth_addr dst_mac;
    str_to_mac(PROBE_GEN_DST_MAC, &dst_mac);
    uint32_t src_ip;
    str_to_ip(PROBE_GEN_SRC_IP, &src_ip);
    uint32_t dst_ip;
    str_to_ip(PROBE_GEN_DST_IP, &dst_ip);
    uint32_t data = PROBE_GEN_DATA;

//        if (!(data >= (thresh_val-thresh) && data <= (thresh_val+thresh))) {
    if (!probe_pkt) {
        probe_pkt = compose_probe_pkt(src_mac, dst_mac, src_ip, dst_ip, data);
    } else {
        struct eth_header *eth = dp_packet_l2(probe_pkt);
        eth->eth_src = src_mac;
        eth->eth_dst = dst_mac;
        struct ip_header *ip = ip = dp_packet_l3(probe_pkt);
        put_16aligned_be32(&ip->ip_src, htonl(src_ip));
        put_16aligned_be32(&ip->ip_dst, htonl(dst_ip));
        ip->ip_csum = 0;
        struct probe_header *prb = dp_packet_l4(probe_pkt);
        prb->data = data;
        ip->ip_csum = csum(ip, sizeof *ip);
    }

    //    struct flow flow;
    //    flow_extract(probe_pkt, &flow);

    gp_execute_action(dp, probe_pkt, output);
//        }

//        thresh_val = data;
}

/*
 * Thread that runs to generate probe every PROBE_GEN_INTERVAL seconds.
 */
static void *
probe_generator(void *dummy OVS_UNUSED) {

    pthread_detach(pthread_self());

    for (;;) {
        ovs_mutex_lock(&probe_gen_mutex);
        odp_execute_send_probe(X,X);
        VLOG_INFO_RL(&rl, "Sending Probe");
        ovs_mutex_unlock(&probe_gen_mutex);
        xsleep(PROBE_GEN_INTERVAL);
    }

    return NULL;
}

int
probe_generator_init(int argc, char **argv) {
    // TODO: Use cmd arguments and configure generator.
    ovs_thread_create("probe_generator", probe_generator, NULL);
    return 0;
}

