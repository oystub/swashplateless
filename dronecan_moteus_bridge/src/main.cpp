#include <array>
#include <iostream>
#include <cstring>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <net/if.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "canard_interface.hpp"
#include "tunnel_node.hpp"
#include "util.hpp"

DEFINE_HANDLER_LIST_HEADS();
DEFINE_TRANSFER_OBJECT_HEADS();

int main(int argc, char** argv)
{
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <moteus_iface> <dronecan_iface>\n";
        return 1;
    }

    MoteusInterface moteus(argv[1]);
    if (!moteus.init()) {
        return 1;
    }

    TunnelNode node;
    node.attachMoteus(&moteus);
    node.init(argv[2]);

    uint64_t next_1hz = micros64();

    while (true) {
        node.processCanard(1);

        canfd_frame frame{};
        while (moteus.readFrameOnce(frame)) {
            uint8_t source      = (frame.can_id >> 8) & 0xFF;
            uint8_t destination = frame.can_id & 0x7F;
            uint8_t len         = (uint8_t)frame.len;
            
            node.tunnel_data(source, destination, frame.data, len);
        }

        uint64_t now = micros64();
        if (now >= next_1hz) {
            next_1hz += 1000000ULL;
            node.process1HzTasks();
        }
    }

    return 0;
}
