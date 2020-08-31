"""a colection of receivers used in drivers, for debugging, actually used receiver is writen in c++"""

import socket
import time
import threading
from . import utilz as u


class DummyDriverReceiver(threading.Thread):
    """
    no docs, again... too bad!

    example:
    t = DummyDriverReceiver(57, addr='192.168.31.60', port=6969)

    with t:
        t.send('hello') # driver id message
        for _ in range(10):
            time.sleep(1) # delay, doesn't affect the receiving
            print (t.get_pose())
        t.send('nut') # whatever send message

    """

    def __init__(self, expected_pose_size, *, addr="127.0.0.1", port=6969):
        """
        ill let you guess what this does
        """
        super().__init__()
        self.eps = expected_pose_size

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((addr, port))
        self.sock.settimeout(2)

        self.backBuffer = bytearray()
        self.readSize = 50
        self._terminator = b"\n"

        self.newPose = [0 for _ in range(self.eps)]

        # -------------------threading related------------------------

        self.alive = True
        self._lock = threading.Lock()
        self.daemon = False

    def __enter__(self):
        self.start()
        if not self.alive:
            self.close()
            raise RuntimeError("receive thread already finished")

        return self

    def __exit__(self, *exp):
        self.stop()

    def stop(self):
        self.alive = False
        self.join(4)
        time.sleep(0.01)
        self.close()

    def close(self):
        """hammer time!"""
        self.sock.send(b"CLOSE\n")
        time.sleep(1)
        self.sock.close()

    def get_pose(self):
        return self.newPose

    def send(self, text):
        self.sock.send(u.format_str_for_write(text))

    def _handlePacket(self, lastPacket):
        lastPacket = lastPacket.decode("utf-8")
        if self.alive and not u.strings_share_characters(
            lastPacket.lower(), "qwrtyuiopsasdfghjklzxcvbnm><*[]{}()"
        ):
            pose = u.get_numbers_from_text(lastPacket.strip("\n").strip(" "), " ")

            if len(pose) != self.eps:
                print(f"pose size miss match, expected {self.eps}, got {len(pose)}")
                return False

            else:
                self.newPose = pose
                return True

        return False

    def run(self):
        while self.alive:
            try:
                data = self.sock.recv(self.readSize)
                self.backBuffer.extend(data)

                if not data:
                    break

                while self._terminator in self.backBuffer:
                    lastPacket, self.backBuffer = self.backBuffer.split(
                        self._terminator, 1
                    )
                    if not self._handlePacket(lastPacket):
                        print(repr(lastPacket), repr(self.backBuffer))

            except socket.timeout as e:
                pass

            except Exception as e:
                print(f"DummyDriverReceiver receive thread failed: {repr(e)}")
                break

        self.alive = False


class UduDummyDriverReceiver(threading.Thread):
    """
    no docs, again... too bad!

    example:
    t = UduDummyDriverReceiver('h13 c22 c22')

    with t:
        t.send('hello') # driver id message
        for _ in range(10):
            time.sleep(1) # delay, doesn't affect the receiving
            print (t.get_pose())
        t.send('nut') # whatever send message

    """

    def __init__(self, expected_pose_struct, *, addr="127.0.0.1", port=6969):
        """
        ill let you guess what this does, :expected_pose_struct: should completely match this regex: ([htc][0-9]+[ ])*([htc][0-9]+)$
        """
        super().__init__()
        self.device_order, self.eps = u.get_pose_struct_from_text(expected_pose_struct)

        if not self.eps:
            raise RuntimeError(f"invalid expected_pose_struct: {expected_pose_struct}")

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((addr, port))
        self.sock.settimeout(2)

        self.backBuffer = bytearray()
        self.readSize = 50
        self._terminator = b"\n"

        self.newPoses = [tuple(0 for _ in range(i)) for i in self.eps]

        # -------------------threading related------------------------

        self.alive = True
        self._lock = threading.Lock()
        self.daemon = False

    def __enter__(self):
        self.start()
        if not self.alive:
            self.close()
            raise RuntimeError("receive thread already finished")

        return self

    def __exit__(self, *exp):
        self.stop()

    def stop(self):
        self.alive = False
        self.join(4)
        time.sleep(0.01)
        self.close()

    def close(self):
        """hammer time!"""
        self.sock.send(b"CLOSE\n")
        time.sleep(1)
        self.sock.close()

    def get_pose(self):
        return self.newPoses

    def send(self, text):
        self.sock.send(u.format_str_for_write(text))

    def _handlePacket(self, lastPacket):
        lastPacket = lastPacket.decode("utf-8")
        if self.alive and not u.strings_share_characters(
            lastPacket.lower(), "qwrtyuiopsasdfghjklzxcvbnm><*[]{}()"
        ):
            poses = u.parse_poses_from_packet(
                u.get_numbers_from_text(lastPacket.strip("\n").strip(" "), " "),
                self.eps,
            )

            if u.get_poses_shape(poses) != self.eps:
                print(
                    f"pose size miss match, expected {self.eps}, got {u.get_poses_shape(poses)}"
                )
                return False

            else:
                self.newPoses = poses
                return True

        return False

    def run(self):
        while self.alive:
            try:
                data = self.sock.recv(self.readSize)
                self.backBuffer.extend(data)

                if not data:
                    break

                while self._terminator in self.backBuffer:
                    lastPacket, self.backBuffer = self.backBuffer.split(
                        self._terminator, 1
                    )
                    if not self._handlePacket(lastPacket):
                        print(repr(lastPacket), repr(self.backBuffer))

            except socket.timeout as e:
                pass

            except Exception as e:
                print(f"UduDummyDriverReceiver receive thread failed: {repr(e)}")
                break

        self.alive = False
