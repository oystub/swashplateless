#include "canard_interface.hpp"

#include <cerrno>
#include <cstring>
#include <unistd.h>

#include <iostream>

#include "util.hpp"

void CanardInterface::init(const char* interface_name)
{
    int16_t res = socketcanInit(&socketcan, interface_name);
    if (res < 0) {
        std::cerr << "Failed to open CAN iface '" << interface_name << "'\n";
        exit(1);
    }

    canardInit(&canard,
               memory_pool,
               sizeof(memory_pool),
               onTransferReceived,
               shouldAcceptTransfer,
               this);
}

bool CanardInterface::broadcast(const Canard::Transfer &bcast_transfer)
{
    tx_transfer = {
        .transfer_type       = bcast_transfer.transfer_type,
        .data_type_signature = bcast_transfer.data_type_signature,
        .data_type_id        = bcast_transfer.data_type_id,
        .inout_transfer_id   = bcast_transfer.inout_transfer_id,
        .priority            = bcast_transfer.priority,
        .payload             = (const uint8_t*)bcast_transfer.payload,
        .payload_len         = uint16_t(bcast_transfer.payload_len),
#if CANARD_ENABLE_DEADLINE
        .deadline_usec       = micros64() + (bcast_transfer.timeout_ms * 1000),
#endif
    };
    return canardBroadcastObj(&canard, &tx_transfer) > 0;
}

bool CanardInterface::request(uint8_t destination_node_id, const Canard::Transfer &req_transfer)
{
    tx_transfer = {
        .transfer_type       = req_transfer.transfer_type,
        .data_type_signature = req_transfer.data_type_signature,
        .data_type_id        = req_transfer.data_type_id,
        .inout_transfer_id   = req_transfer.inout_transfer_id,
        .priority            = req_transfer.priority,
        .payload             = (const uint8_t*)req_transfer.payload,
        .payload_len         = uint16_t(req_transfer.payload_len),
#if CANARD_ENABLE_DEADLINE
        .deadline_usec       = micros64() + (req_transfer.timeout_ms * 1000),
#endif
    };
    return canardRequestOrRespondObj(&canard, destination_node_id, &tx_transfer) > 0;
}

bool CanardInterface::respond(uint8_t destination_node_id, const Canard::Transfer &res_transfer)
{
    tx_transfer = {
        .transfer_type       = res_transfer.transfer_type,
        .data_type_signature = res_transfer.data_type_signature,
        .data_type_id        = res_transfer.data_type_id,
        .inout_transfer_id   = res_transfer.inout_transfer_id,
        .priority            = res_transfer.priority,
        .payload             = (const uint8_t*)res_transfer.payload,
        .payload_len         = uint16_t(res_transfer.payload_len),
#if CANARD_ENABLE_DEADLINE
        .deadline_usec       = micros64() + (res_transfer.timeout_ms * 1000),
#endif
    };
    return canardRequestOrRespondObj(&canard, destination_node_id, &tx_transfer) > 0;
}

void CanardInterface::process(uint32_t timeout_msec)
{
    // Transmit
    for (const CanardCANFrame* txf = nullptr; (txf = canardPeekTxQueue(&canard)) != nullptr; ) {
        const int16_t tx_res = socketcanTransmit(&socketcan, txf, 0);  // non-blocking
        if (tx_res > 0) {
            // Queued OK -> remove from canard TX queue
            canardPopTxQueue(&canard);
        } else if (tx_res == 0) {
            // Would block -> stop trying this cycle; try again next process()
            break;
        } else { 
            // tx_res < 0 -> error
            if (errno == ENOBUFS) {
                // Kernel TX queue full -> back off and retry later
                usleep(100);
                break;
            } else {
                std::cerr << "socketcanTransmit error: " << tx_res 
                        << " (errno=" << errno << ": " << strerror(errno) << ")\n";
                break;
            }
        }
    }
    // Receive
    uint32_t start_ms = millis32();
    while (millis32() - start_ms < timeout_msec) {
        CanardCANFrame rx_frame;
        int16_t rx_res = socketcanReceive(&socketcan, &rx_frame, timeout_msec);
        if (rx_res > 0) {
            auto res = canardHandleRxFrame(&canard, &rx_frame, micros64());
            switch (res) {
                case -CANARD_ERROR_RX_MISSED_START:
                case -CANARD_ERROR_RX_WRONG_TOGGLE:
                case -CANARD_ERROR_RX_SHORT_FRAME:
                case -CANARD_ERROR_RX_BAD_CRC:
                    std::cout << "CAN RX error: " << res << "\n";
                    break;
                default:
                    break;
            }
        }
    }
}

void CanardInterface::onTransferReceived(CanardInstance* ins, CanardRxTransfer* transfer)
{
    // The user_reference is our 'this' pointer
    CanardInterface* iface = static_cast<CanardInterface*>(ins->user_reference);
    // Dispatch to the standard handler logic (C++ classes)
    iface->handle_message(*transfer);
}

bool CanardInterface::shouldAcceptTransfer(const CanardInstance* ins,
                                           uint64_t* out_data_type_signature,
                                           uint16_t data_type_id,
                                           CanardTransferType transfer_type,
                                           uint8_t source_node_id)
{
    // The user_reference is our 'this' pointer
    CanardInterface* iface = (CanardInterface*)ins->user_reference;
    return iface->accept_message(data_type_id, transfer_type, *out_data_type_signature);
}