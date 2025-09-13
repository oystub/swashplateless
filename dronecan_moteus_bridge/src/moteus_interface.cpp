#include "moteus_interface.hpp"

#include <iostream>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>

// Constructor
MoteusInterface::MoteusInterface(const std::string& interface_name)
    : interface_name_(interface_name), socket_fd_(-1) {}

MoteusInterface::~MoteusInterface() {
    if (socket_fd_ != -1) {
        close(socket_fd_);
    }
}

bool MoteusInterface::init() {
    struct ifreq ifr{};
    struct sockaddr_can addr{};

    socket_fd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socket_fd_ < 0) {
        std::cerr << "Error opening Moteus socket\n";
        return false;
    }

    // Enable CAN FD
    int enable_canfd = 1;
    if (setsockopt(socket_fd_, SOL_CAN_RAW, CAN_RAW_FD_FRAMES,
                   &enable_canfd, sizeof(enable_canfd)) < 0) {
        std::cerr << "Error enabling CAN FD mode for Moteus\n";
        return false;
    }

    // Set non-blocking mode
    int flags = fcntl(socket_fd_, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK);
    }

    std::strncpy(ifr.ifr_name, interface_name_.c_str(), IFNAMSIZ - 1);
    if (ioctl(socket_fd_, SIOCGIFINDEX, &ifr) < 0) {
        std::cerr << "Error getting interface index for Moteus\n";
        return false;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(socket_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Error binding Moteus socket\n";
        return false;
    }
    return true;
}

bool MoteusInterface::readFrameOnce(canfd_frame& frameOut) {
    ssize_t num_bytes = ::read(socket_fd_, &frameOut, sizeof(frameOut));
    return num_bytes == static_cast<ssize_t>(sizeof(canfd_frame));
}

bool MoteusInterface::sendFrame(const canfd_frame& frame) {
    ssize_t num_bytes = ::write(socket_fd_, &frame, sizeof(frame));
    if (num_bytes < 0) {
        std::cerr << "Error sending CAN frame to Moteus\n";
        return false;
    }
    return true;
}
