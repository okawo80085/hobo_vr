import socket
import time
import threading
from . import utilz as u

class DummyDriver(threading.Thread):
    """
    no docs, again... too bad!

    example:
    t = DummyDriver(57, addr='192.168.31.60', port=6969)

    with t:
        t.send('hello') # driver id message
        for _ in range(10):
            time.sleep(1) # delay, doesn't affect the receiving
            print (t.get_pose())
        t.send('nut') # whatever send message


    """

    def __init__(self, expected_pose_size, *, addr='127.0.0.1', port=6969):
        '''
        ill let you guess what this does
        '''
        super().__init__()
        self.eps = expected_pose_size

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((addr, port))
        self.sock.settimeout(2)

        # -------------------threading related------------------------

        self.alive = True
        self._lock = threading.Lock()
        self.daemon = False

    def __enter__(self):
        self.start()
        if not self.alive:
            self.close()
            raise RuntimeError("video source already expired")

        return self

    def __exit__(self, *exp):
        self.stop()

    def stop(self):
        self.alive = False
        self.join(4)
        time.sleep(0.01)
        self.close()

    def close(self):
        '''hammer time!'''
        self.sock.send(b'CLOSE\n')
        time.sleep(1)
        self.sock.close()

    def get_pose(self):
        if self.alive:
            pose = u.get_numbers_from_text(self.lastBuff, ' ')

            if len(pose) != self.eps:
                pose = [0 for _ in range(self.eps)]

            return pose

        else:
            raise RuntimeError("recv thread already finished")

    def send(self, text):
        self.sock.send(u.format_str_for_write(text))

    def run(self):
        while self.alive:
            try:
                data = u.read2(self.sock)

                if not data:
                    break

                self.lastBuff = data

            except socket.timeout as e:
                pass

            except Exception as e:
                print (f'DummyDriver recv thread failed: {repr(e)}')
                break

        self.alive = False
