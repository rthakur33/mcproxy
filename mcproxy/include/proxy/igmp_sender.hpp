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

/**
 * @addtogroup mod_sender Sender
 * @{
 */

#ifndef IGMP_SENDER_HPP
#define IGMP_SENDER_HPP

#include "include/proxy/sender.hpp"

/**
 * @brief Generates IGMP messages.
 */
class igmp_sender : public sender
{
private:
    enum msg_type { //for intern type handling
        GENERAL_QUERY, GROUP_SPECIFIC_QUERY
    };

    bool create_mc_query(msg_type type, unsigned char* buf, const addr_storage* g_addr = nullptr) const;
    int get_msg_min_size() const;
public:
    /**
     * @brief Create an igmp_sender.
     */
    igmp_sender();

    bool init(int addr_family) override;

    bool send_general_query(int if_index) const override;
    bool send_group_specific_query(int if_index, const addr_storage& g_addr) const override;
    bool send_report(int if_index, const addr_storage& g_addr) const override;
    bool send_leave(int if_index, const addr_storage& g_addr) const override;
};

#endif // IGMP_SENDER_HPP
/** @} */
