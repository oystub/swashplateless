#include "tunnel_node.hpp"

#include <linux/can.h>

#include <cstring>
#include <algorithm>



// Constructor
TunnelNode::TunnelNode()
    : canard_iface(0)
{
    memset(&node_status_msg, 0, sizeof(node_status_msg));
}

void TunnelNode::attachMoteus(MoteusInterface* moteus) {
    moteus_ptr = moteus;
}

void TunnelNode::init(const char* dronecan_iface) {
    canard_iface.init(dronecan_iface);
    canard_iface.set_node_id(MY_NODE_ID);
}

void TunnelNode::process1HzTasks() {
    send_NodeStatus();
}

void TunnelNode::tunnel_data(uint8_t source, uint8_t destination, const uint8_t* data, uint8_t len)
{
    constexpr uint8_t max_buf = sizeof(decltype(((uavcan_tunnel_Broadcast*)0)->buffer.data));
    constexpr uint8_t overhead = 2; // source + destination
    constexpr uint8_t max_payload = max_buf - overhead - 1;

    uint8_t offset = 0;

    while (offset < len || (len == 0 && offset == 0)) {
        uavcan_tunnel_Broadcast msg{};
        msg.protocol.protocol = UAVCAN_TUNNEL_PROTOCOL_UNDEFINED;
        msg.channel_id = 0;
        msg.buffer.data[0] = source;
        msg.buffer.data[1] = destination;

        uint8_t remaining = len - offset;
        size_t chunk_len = std::min<size_t>(remaining, max_payload);

        memcpy(msg.buffer.data + overhead, data + offset, chunk_len);
        msg.buffer.len = chunk_len + overhead;

        if (remaining > max_payload) {
            msg.buffer.len = max_buf;
            msg.buffer.data[max_buf - 1] = 0;
        }

        tunnel_broadcast.broadcast(msg);
        processCanard(0);

        offset += chunk_len;
    }
}

void TunnelNode::processCanard(uint32_t timeout_msec) {
    canard_iface.process(timeout_msec);
}

void TunnelNode::handle_tunnel_broadcast(const CanardRxTransfer&, const uavcan_tunnel_Broadcast& msg)
{
    if (!moteus_ptr) return;
    if (msg.buffer.len < 2) return;
    if (msg.protocol.protocol != UAVCAN_TUNNEL_PROTOCOL_UNDEFINED || msg.channel_id != 0) return;

    uint8_t source = msg.buffer.data[0];
    uint8_t destination = msg.buffer.data[1];

    if (reassembly_len == 0) {
        reassembly_source = source;
        reassembly_dest = destination;
    }

    size_t payload_len = msg.buffer.len - 2;

    if (msg.buffer.len == sizeof(msg.buffer.data) && payload_len > 0) {
        payload_len -= 1;
    }

    if (reassembly_len + payload_len <= reassembly_buf.size()) {
        memcpy(&reassembly_buf[reassembly_len],
               &msg.buffer.data[2],
               payload_len);
        reassembly_len += payload_len;
    } else {
        reassembly_len = 0;
        return;
    }

    if (msg.buffer.len < sizeof(msg.buffer.data)) {
        canfd_frame f{};
        uint16_t can_id = ((uint16_t)(reassembly_source & 0x7F) << 8) |
                          (reassembly_dest & 0x7F);
        f.can_id = can_id;
        f.len = (reassembly_len > 64) ? 64 : (uint8_t)reassembly_len;
        memcpy(f.data, reassembly_buf.data(), f.len);

        moteus_ptr->sendFrame(f);

        reassembly_len = 0;
    }
}

void TunnelNode::handle_GetNodeInfo(const CanardRxTransfer& transfer, const uavcan_protocol_GetNodeInfoRequest&)
{
    uavcan_protocol_GetNodeInfoResponse node_info_rsp{};
    node_info_rsp.name.len = snprintf((char*)node_info_rsp.name.data,
                                      sizeof(node_info_rsp.name.data),
                                      "Moteus Tunnel Node");

    node_info_rsp.software_version.major = 1;
    node_info_rsp.software_version.minor = 0;
    node_info_rsp.hardware_version.major = 1;
    node_info_rsp.hardware_version.minor = 0;
    getUniqueID(node_info_rsp.hardware_version.unique_id);

    node_info_rsp.status = node_status_msg;
    node_info_rsp.status.uptime_sec = millis32() / 1000UL;

    node_info_server.respond(transfer, node_info_rsp);
}

void TunnelNode::send_NodeStatus() {
    node_status_msg.health = UAVCAN_PROTOCOL_NODESTATUS_HEALTH_OK;
    node_status_msg.mode   = UAVCAN_PROTOCOL_NODESTATUS_MODE_OPERATIONAL;
    node_status_msg.sub_mode = 0;
    node_status_msg.uptime_sec = millis32() / 1000UL;

    node_status.broadcast(node_status_msg);
}
