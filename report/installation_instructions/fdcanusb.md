# Using the fdcanusb/mjcanfd-usb-1x

The fdcanusb/mjcanfd-usb-1x are CAN to USB adapters produced by mjbots. As of writing, the fdcanusb is no longer available for sale on the moteus website, and the mjcanfd-usb-1x can therefore be seen as a successor. Both devices run the same open source firmware [mjbots/fdcanusb](https://github.com/mjbots/fdcanusb). They present themselves as a USB CDC device, with a custom text-based protocol, similar to SLCAN, but not directly compatible.

## Using fdcansb/mjcanfd for moteus protocol
The devices are directly compatible with the moteus toolig like `moteus_tool` and `moteus_gui`. As long as the device is plugged in, it will be automatically detected by 

## Using fdcanusb/mjcanfd for DroneCAN
Since the adapter communicates with the host computer using a custom protocol, it is not directly compatible with common DroneCAN utilities such as DroneCAN GUI, pydronecan or libcanard.

However, moteus also supply an open source program, `fdcanusb_daemon`, which bridges the moteus interface to a SocketCAN virtual can adapter. SocketCAN is part of the Linux kernel and widely supported by most open source CAN projects, including for DroneCAN.

A pre-built `fdcanusb_daemon` is provided with the released software. To use it:

Create a virtual can port
```
sudo ip link add name vcan0 type vcan
sudo ip link set vcan0 up
```
Where `vcan0` can be chosen freely, but a name on the format `vcan<number>` is recommended, as this makes it autodetected by some utilities like DroneCAN GUI.

Then run the program using
```
./fdcanusb_daemon /dev/ttyAMA0 vcan0
```
Where `/dev/ttyAMA0` is the serial interface of the fdcanusb adapter (might differ on your computer), and `vcan0` is the name of the SocketCAN interface created in the previous step.

> **Tip:** Predictable serial port names
> 
> To get predictable names for the fdcanusb/mjcanfd adapters' serial port, you can add the following udev rule:
> 
> `SUBSYSTEM=="tty", ATTRS{manufacturer}=="mjbots", ATTRS{product}=="fdcanusb", MODE="0666", SYMLINK+="fdcanusb"`
> 
> With this rule, the adapter will always be available 
> 

After this, the `vcan0` interface can be used as a transparent bridge to the CAN bus connected to the fdcanusb/mjcanfd adapter. 
