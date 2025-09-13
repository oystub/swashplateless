# DroneCAN moteus bridge
This tool provides a bridge between two virtual CAN buses:
- One bus that carries only DroneCAN frames
- One bus that carries moteus-specific frame formats

Because these two formats are not compatible, they normally cannot share the same CAN bus directly.
The tunnel resolves this by encapsulating moteus frames inside DroneCAN `uavcan.tunnel.Broadcast messages`, and by unwrapping those messages on the other side.

This allows standard moteus software tools, such as:
- moteus_tool
- tview

to work with a DroneCAN-enabled moteus controller without modification.

When using these tools, with the tunnel running, simply append to the desired command:
```
--can-interface=socketcan --can-chan=<moteus_bus>
```

## How It Works

The program takes two virtual CAN bus devices as arguments:
- A DroneCAN-compliant bus (e.g. `vcan0`)
- A moteus-format bus (e.g. `vcan1`)

The tunnel provides:
- Encapsulation: moteus frames are wrapped into DroneCAN `uavcan.tunnel.Broadcast` messages and transmitted over the DroneCAN bus.
- Decapsulation: tunnel messages received from DroneCAN are unpacked and forwarded onto the moteus bus.

In practice, this makes the moteus bus appear as if the controller were directly attached, even though communication is actually routed through the DroneCAN network.

## Build Instructions
### Requirements
- Git, C/C++ toolchain, CMake
- Python 3 with venv (for DSDL code generation)
- (Recommended) `can-utils` to create virtual CAN devices for testing

On Ubuntu/Debian:
```
sudo apt-get update
sudo apt-get install -y git build-essential cmake python3 python3-pip python3-venv can-utils
```

### Initialize submodules
```
git submodule update --init --recursive
```

### Configure and build
CMake generates DSDL code into the build directory during configure, then builds everything.
```
mkdir build
cd build
cmake ..
make
```

### Run

Create a virtual CAN bus for moteus frames
```
sudo modprobe vcan
sudo ip link add dev vcan_moteus type vcan
sudo ip link set up vcan_moteus
```

Run:
```
./dronecan_moteus_bridge vcan_moteus <dronecan_interface>
```

Where `<dronecan_interface>` is the socketcan device for DroneCAN.

I use a Zubax Babel, and use 
```
sudo slcand -o -c -f -s8 -F /dev/serial/by-id/usb-Zubax_Robotics_Zubax_Babel_[...] vcan0
```
To create a socketcan interface on `vcan0` for the DroneCAN bus.
