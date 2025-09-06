import asyncio
import signal
import io
from moteus.moteus import Writer, Result
import moteus


async def run_motor(c: moteus.Controller, velocity: float):
    """Main loop for controlling the motor."""
    while True:
        data_buf = io.BytesIO()
        writer = Writer(data_buf)

        # Set mode to sinusoidal velocity
        writer.write_varuint(0x00 | 0x01)  # Write one u8
        writer.write_varuint(int(moteus.Register.MODE))
        writer.write_int8(16)  # Sinusoidal velocity

        # Velocity
        writer.write_varuint(0x0c | 0x01)  # Write one f32
        writer.write_varuint(int(moteus.Register.COMMAND_VELOCITY))
        writer.write_f32(velocity)

        # Sinusoidal write_varuint
        writer.write_varuint(0x0c | 0x03)  # Write three f32
        writer.write_varuint(0x160)
        writer.write_f32(0.0)  # Scale
        writer.write_f32(0.0)  # Phase
        writer.write_f32(0.0)  # Feedforward

        cmd = c.make_query()
        cmd.data = data_buf.getvalue()

        await c.execute(cmd)

        await asyncio.sleep(0.02)


async def query_motor(c: moteus.Controller):
    while True:
        """Query for velocity and I term"""
        data_buf = io.BytesIO()
        writer = Writer(data_buf)

        # Query mode
        writer.write_varuint(0x10 | 0x01)  # One int8
        writer.write_varuint(int(moteus.Register.MODE))

        # Query velocity
        writer.write_varuint(0x1c | 0x01)  # One f32
        writer.write_varuint(int(moteus.Register.VELOCITY))

        # Query I term
        writer.write_varuint(0x1c | 0x01)  # One f32
        writer.write_varuint(int(moteus.Register.POSITION_KI))

        cmd = c.make_query()
        cmd.data = data_buf.getvalue()

        res = await c.execute(cmd)

        if res is None:
            print("No response from motor")
            await asyncio.sleep(0.5)
            continue

        velocity = res.values[moteus.Register.VELOCITY]
        ki = -res.values[moteus.Register.POSITION_KI]

        print(f"Velocity: {velocity:.3f} rps, I term: {ki:.3f} Nm")
        await asyncio.sleep(0.5)


async def main():
    transport = moteus.PythonCan(
        interface="socketcan",
        channel="vcan1",
        fd=True,
    )

    c = moteus.Controller(id=1, transport=transport)

    # Clear any faults.
    await c.set_stop()

    loop = asyncio.get_running_loop()
    stop_event = asyncio.Event()

    def handle_sigint():
        stop_event.set()

    # Register SIGINT handler
    loop.add_signal_handler(signal.SIGINT, handle_sigint)

    motor_task = asyncio.create_task(run_motor(c, 5))
    query_task = asyncio.create_task(query_motor(c))

    # Wait for Ctrl-C
    await stop_event.wait()

    print("\nStopping motor...")
    motor_task.cancel()
    query_task.cancel()
    try:
        await motor_task
    except asyncio.CancelledError:
        pass

    await c.set_stop()
    print("Motor stopped, exiting cleanly.")


if __name__ == "__main__":
    asyncio.run(main())
