#pragma once

#include <array>
#include <cstdint>
#include <canard/publisher.h>
#include <canard/subscriber.h>
#include <canard/service_server.h>
#include <dronecan_msgs.h>

#include "canard_interface.hpp"
#include "moteus_interface.hpp"
#include "util.hpp"

#define MY_NODE_ID 126

class TunnelNode {
public:
    TunnelNode();

    void attachMoteus(MoteusInterface* moteus);
    void init(const char* dronecan_iface);

    void process1HzTasks();

    void tunnel_data(uint8_t source, uint8_t destination, const uint8_t* data, uint8_t len);

    void processCanard(uint32_t timeout_msec);

private:
    std::array<uint8_t, 64> reassembly_buf{};
    size_t reassembly_len = 0;
    uint8_t reassembly_source = 0;
    uint8_t reassembly_dest = 0;

    CanardInterface canard_iface;
    MoteusInterface* moteus_ptr = nullptr;
    
    void handle_tunnel_broadcast(const CanardRxTransfer& transfer, const uavcan_tunnel_Broadcast& msg);
    Canard::ObjCallback<TunnelNode, uavcan_tunnel_Broadcast> tunnel_broadcast_cb{this, &TunnelNode::handle_tunnel_broadcast};
    Canard::Subscriber<uavcan_tunnel_Broadcast> tunnel_broadcast_sub{tunnel_broadcast_cb, 0};

    void handle_GetNodeInfo(const CanardRxTransfer& transfer, const uavcan_protocol_GetNodeInfoRequest& req);
    Canard::ObjCallback<TunnelNode, uavcan_protocol_GetNodeInfoRequest> node_info_req_cb{this, &TunnelNode::handle_GetNodeInfo};
    Canard::Server<uavcan_protocol_GetNodeInfoRequest> node_info_server{canard_iface, node_info_req_cb};

    Canard::Publisher<uavcan_tunnel_Broadcast> tunnel_broadcast{canard_iface};

    void send_NodeStatus();
    uavcan_protocol_NodeStatus node_status_msg{};
    Canard::Publisher<uavcan_protocol_NodeStatus> node_status{canard_iface};
};