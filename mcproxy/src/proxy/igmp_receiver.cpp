/*
 * This file is part of mcproxy.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * written by Sebastian Woelke, in cooperation with:
 * INET group, Hamburg University of Applied Sciences,
 * Website: http://mcproxy.realmv6.org/
 */

#include "include/hamcast_logging.h"
#include "include/proxy/igmp_receiver.hpp"
#include "include/proxy/proxy_instance.hpp"
#include "include/proxy/message_format.hpp"
#include "include/utils/extended_igmp_defines.hpp"

#include <net/if.h>
#include <linux/mroute.h>
#include <netinet/igmp.h>
#include <netinet/ip.h>



igmp_receiver::igmp_receiver(proxy_instance* pr_i, const std::shared_ptr<const mroute_socket> mrt_sock, const std::shared_ptr<const interfaces> interfaces, bool in_debug_testing_mode): receiver(pr_i, AF_INET, mrt_sock, interfaces, in_debug_testing_mode)
{
    HC_LOG_TRACE("");

    start();
}

int igmp_receiver::get_iov_min_size()
{
    HC_LOG_TRACE("");
    int size_ip = sizeof(struct ip) + sizeof(struct igmp) + IGMP_RECEIVER_IPV4_ROUTER_ALERT_OPT_SIZE + 10000; //random number plz fix it??????????????????????????
    int size_kernel = sizeof(struct igmpmsg);
    return (size_ip < size_kernel) ? size_kernel : size_ip;
}

int igmp_receiver::get_ctrl_min_size()
{
    HC_LOG_TRACE("");
    //useless
    return 0;
}

void igmp_receiver::analyse_packet(struct msghdr* msg, int)
{
    HC_LOG_TRACE("");

    struct ip* ip_hdr = (struct ip*)msg->msg_iov->iov_base;
    struct igmp* igmp_hdr = (struct igmp*) ((char*)msg->msg_iov->iov_base + ip_hdr->ip_hl * 4);

    unsigned int if_index = 0;
    addr_storage gaddr;
    addr_storage saddr;
    //proxy_instance* pr_i;

    //test_output::printPacket_IPv4_Infos(buf, msg_size);
    HC_LOG_DEBUG("received packet XXXXXXXXXXXXXXXXXXXXXXXXXXXXX");

    /* packets sent up from kernel to daemon have ip->ip_p = 0 */
    if (ip_hdr->ip_p == IGMP_RECEIVER_KERNEL_MSG) {
        HC_LOG_DEBUG("kernel msg");
        struct igmpmsg *igmpctl;
        igmpctl = (struct igmpmsg *) msg->msg_iov->iov_base;

        switch (igmpctl->im_msgtype) {
        case IGMPMSG_NOCACHE: {
            saddr = igmpctl->im_src;
            HC_LOG_DEBUG("\tsrc: " << saddr);

            gaddr = igmpctl->im_dst;
            HC_LOG_DEBUG("\tgroup: " << gaddr);

            HC_LOG_DEBUG("\tvif: " << (int)igmpctl->im_vif);
            if ((if_index = m_interfaces->get_if_index(igmpctl->im_vif)) == 0) {
                return;
            }

            if (!is_if_index_relevant(if_index)) { //I think this is not nessessary (see line above)???????????????? and next message types
                return;
            }

            HC_LOG_DEBUG("\tif_index: " << if_index);
            m_proxy_instance->add_msg(std::make_shared<new_source_msg>(if_index, gaddr, saddr));
            break;
        }
        default:
            HC_LOG_WARN("unknown kernel message");
        }
    } else if (ip_hdr->ip_p == IPPROTO_IGMP && ntohs(ip_hdr->ip_len) <= get_iov_min_size()) { //???????
        //test_output::printPaket_IPv4_IgmpInfos(buf);
        if (igmp_hdr->igmp_type == IGMP_V2_MEMBERSHIP_REPORT) {
            HC_LOG_DEBUG("\tjoin");

            saddr = ip_hdr->ip_src;
            HC_LOG_DEBUG("\tsrc: " << saddr);

            gaddr = igmp_hdr->igmp_group;
            HC_LOG_DEBUG("\tgroup: " << gaddr);

            if ((if_index = m_interfaces->get_if_index(saddr)) == 0) {
                return;
            }
            HC_LOG_DEBUG("\tif_index: " << if_index);

            //if ((pr_i = this->get_proxy_instance(if_index)) == nullptr) {
            //return;
            //}

            //proxy_msg m;
            //m.type = proxy_msg::RECEIVER_MSG;
            //m.msg = new struct receiver_msg(receiver_msg::JOIN, if_index, g_addr);
            //pr_i->add_msg(m);
            HC_LOG_ERROR("protocoll not supported igmpv2 mem report");
        } else if (igmp_hdr->igmp_type == IGMP_V2_LEAVE_GROUP) {
            HC_LOG_DEBUG("\tleave");

            saddr = ip_hdr->ip_src;
            HC_LOG_DEBUG("\tsrc: " << saddr);

            gaddr = igmp_hdr->igmp_group;
            HC_LOG_DEBUG("\tgroup: " << gaddr);

            if ((if_index = m_interfaces->get_if_index(saddr)) == 0) {
                return ;
            }
            HC_LOG_DEBUG("\tif_index: " << if_index);

            //if ((pr_i = this->get_proxy_instance(if_index)) == nullptr) {
            //return;
            //}

            //proxy_msg m;
            //m.type = proxy_msg::RECEIVER_MSG;
            //m.msg = new struct receiver_msg(receiver_msg::LEAVE, if_index, g_addr);
            //pr_i->add_msg(m);

            HC_LOG_ERROR("protocoll not supported igmpv2 leave group");
        } else if (igmp_hdr->igmp_type == IGMP_V3_MEMBERSHIP_REPORT) {
            HC_LOG_DEBUG("IGMP_V3_MEMBERSHIP_REPORT found");
            igmpv3_mc_report* v3_report = reinterpret_cast<igmpv3_mc_report*>(igmp_hdr);
            igmpv3_mc_record* rec = reinterpret_cast<igmpv3_mc_record*>(reinterpret_cast<unsigned char*>(v3_report) + sizeof(igmpv3_mc_report));

            int num_records = ntohs(v3_report->num_of_mc_records);
            HC_LOG_DEBUG("num of multicast records: " << num_records);

            saddr = ip_hdr->ip_src;
            HC_LOG_DEBUG("saddr: " << saddr);

            if ((if_index = m_interfaces->get_if_index(saddr)) == 0) {
                HC_LOG_DEBUG("no if_index found");
                return;
            }
            HC_LOG_DEBUG("found on interface:" << interfaces::get_if_name(if_index));

            if (!is_if_index_relevant(if_index)) { //I think this is not nessessary (see line above)???????????????? and next message types
                HC_LOG_DEBUG("interface is not relevant");
                return;
            }

            for (int i = 0; i < num_records; ++i) {
                mcast_addr_record_type rec_type = static_cast<mcast_addr_record_type>(rec->type);
                unsigned int aux_size = rec->aux_data_len * 4; //RFC 3376 Section 4.2.6 Aux Data Len
                int nos = ntohs(rec->num_of_srcs);
                gaddr = addr_storage(rec->gaddr);
                source_list<source> slist;
                for (int j = 0; j < nos; ++j) {
                    in_addr* src = reinterpret_cast<in_addr*>(reinterpret_cast<unsigned char*>(rec) + sizeof(in_addr));
                    slist.insert(addr_storage(*src));
                    src++;
                }

                HC_LOG_DEBUG("record type: " << get_mcast_addr_record_type_name(rec_type));
                HC_LOG_DEBUG("gaddr: " << gaddr);
                HC_LOG_DEBUG("number of sources: " << slist.size());
                HC_LOG_DEBUG("send record to proxy_instance");
                std::cout << "process a IGMP_V3_MEMBERSHIP_REPORT" << std::endl;
                m_proxy_instance->add_msg(std::make_shared<group_record_msg>(if_index, rec_type, gaddr, move(slist), -1));

                rec = reinterpret_cast<igmpv3_mc_record*>(reinterpret_cast<unsigned char*>(rec) + sizeof(igmpv3_mc_record) + nos * sizeof(in_addr) + aux_size);
            }
        } else {
            HC_LOG_DEBUG("unknown IGMP-packet");
            HC_LOG_DEBUG("type: " << igmp_hdr->igmp_type);
        }
    } else {
        HC_LOG_DEBUG("unknown IP-packet: " << ip_hdr->ip_p);
    }
}


