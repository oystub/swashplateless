#pragma once

#include <cstdint>

#include <canard.h>
#include <canard/handler_list.h>
#include <canard/transfer_object.h>
#include <canard/publisher.h>
#include <canard/subscriber.h>
#include <canard/service_client.h>
#include <canard/service_server.h>
#include <socketcan.h>


class CanardInterface : public Canard::Interface {
    friend class TunnelNode;

public:
    CanardInterface(uint8_t iface_index) : Interface(iface_index) {}

    void init(const char* interface_name);

    bool broadcast(const Canard::Transfer &bcast_transfer) override;
    bool request(uint8_t destination_node_id, const Canard::Transfer &req_transfer) override;
    bool respond(uint8_t destination_node_id, const Canard::Transfer &res_transfer) override;

    void process(uint32_t duration_ms);

    static void onTransferReceived(CanardInstance* ins, CanardRxTransfer* transfer);
    static bool shouldAcceptTransfer(const CanardInstance* ins,
                                     uint64_t* out_data_type_signature,
                                     uint16_t data_type_id,
                                     CanardTransferType transfer_type,
                                     uint8_t source_node_id);

    uint8_t get_node_id() const override { return canard.node_id; }
    void set_node_id(uint8_t node_id) {
        canardSetLocalNodeID(&canard, node_id);
    }

private:
    uint8_t memory_pool[8192];
    CanardInstance canard;
    CanardTxTransfer tx_transfer;

    SocketCANInstance socketcan;
};