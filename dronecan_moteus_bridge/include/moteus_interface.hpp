#pragma once

#include <linux/can.h>
#include <linux/can/raw.h>
#include <string>

class MoteusInterface {
public:
    explicit MoteusInterface(const std::string& interface_name);
    ~MoteusInterface();

    bool init();

    // Read one frame, non-blocking. Returns true if a frame was read.
    bool readFrameOnce(canfd_frame& frameOut);

    // Send a CAN FD frame
    bool sendFrame(const canfd_frame& frame);

private:
    std::string interface_name_;
    int socket_fd_;
};
