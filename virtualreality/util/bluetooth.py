import bluetooth
import time

import threading


class BluetoothReceiver(threading.Thread):
    """
    no docs... too bad!

    oh yeah, here is an example:
        addr = '98:D3:51:FD:EB:C4' # some device address, whatever
        port = 1

        t = BluetoothReceiver(addr, port)

        with t:
            for _ in range(1000):
                print (t.last_read)
                time.sleep(1/100) # time delay, doesn't affect receiving, whatever whatever
    """

    def __init__(self, addr, port):
        """
        here is a riddle for you: what does init do?
        """
        super().__init__()
        self.backBuffer = bytearray()
        self._last_read = b""
        self._terminator = b"\n"

        self.sock = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
        self.sock.connect((addr, port))

        self.readSize = 100

        # -------------------threading related------------------------

        self.alive = True
        self._lock = threading.Lock()
        self.daemon = False

    def __enter__(self):
        self.start()
        if not self.alive:
            self.close()
            raise RuntimeError("receive thread already finished")

    def __exit__(self, *exp):
        self.stop()

    def stop(self):
        self.alive = False
        self.join(4)
        time.sleep(0.01)
        self.close()

    def close(self):
        """hammer time!"""
        time.sleep(0.5)
        self.sock.close()

    @property
    def last_read(self):
        return self._last_read

    def send(self, data):
        """sends data or something idk"""
        self.sock.send(data)

    def run(self):
        while self.alive:
            try:
                data = self.sock.recv(self.readSize)
                self.backBuffer.extend(data)

                if not data:
                    break

                while self._terminator in self.backBuffer:
                    self._last_read, self.backBuffer = self.backBuffer.split(
                        self._terminator, 1
                    )

            except Exception as e:
                print(f"BluetoothSocket receive thread failed: {repr(e)}")
                break

        self.alive = False
