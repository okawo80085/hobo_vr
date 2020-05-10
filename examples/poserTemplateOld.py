"""
this is a template of a poser script

modify the Poser class to your needs, except:
1. don't modify the keys in self.pose, self.poseControllerR, self.poseControllerL
2. stay within the value ranges for the keys in self.pose, self.poseControllerR, self.poseControllerL
3. don't modify send() and _socketInit() methods
4. it is not recommended to modify recv() method if you don't know what you're doing
6. to make sure that self.incomingData_readonly could be accessed by any thread at any time it is meant to be read only, if you need to modify self.incomingData_readonly make a copy
5. all values in self.pose, self.poseControllerR and self.poseControllerL need to be numeric

"""

import asyncio
import virtualreality.utilz as u
import time
import keyboard


class Poser:
    def __init__(self, addr="127.0.0.1", port=6969):
        self.pose = {
            # location in meters and orientation in quaternion
            "x": 0,  # +(x) is right in meters
            "y": 1,  # +(y) is up in meters
            "z": 0,  # -(z) is forward in meters
            "rW": 1,  # from 0 to 1
            "rX": 0,  # from -1 to 1
            "rY": 0,  # from -1 to 1
            "rZ": 0,  # from -1 to 1
            # velocity in meters/second
            "velX": 0,  # +(x) is right in meters/second
            "velY": 0,  # +(y) is right in meters/second
            "velZ": 0,  # -(z) is right in meters/second
            # Angular velocity of the pose in axis-angle
            # representation. The direction is the angle of
            # rotation and the magnitude is the angle around
            # that axis in radians/second.
            "angVelX": 0,
            "angVelY": 0,
            "angVelZ": 0,
        }

        self.poseControllerR = {
            # location in meters and orientation in quaternion
            "x": 0.5,  # +(x) is right in meters
            "y": 1,  # +(y) is up in meters
            "z": -1,  # -(z) is forward in meters
            "rW": 1,  # from 0 to 1
            "rX": 0,  # from -1 to 1
            "rY": 0,  # from -1 to 1
            "rZ": 0,  # from -1 to 1
            # velocity in meters/second
            "velX": 0,  # +(x) is right in meters/second
            "velY": 0,  # +(y) is right in meters/second
            "velZ": 0,  # -(z) is right in meters/second
            # Angular velocity of the pose in axis-angle
            # representation. The direction is the angle of
            # rotation and the magnitude is the angle around
            # that axis in radians/second.
            "angVelX": 0,
            "angVelY": 0,
            "angVelZ": 0,
            # inputs
            "grip": 0,  # 0 or 1
            "system": 0,  # 0 or 1
            "menu": 0,  # 0 or 1
            "trackpadClick": 0,  # 0 or 1
            "triggerValue": 0,  # from 0 to 1
            "trackpadX": 0,  # from -1 to 1
            "trackpadY": 0,  # from -1 to 1
            "trackpadTouch": 0,  # 0 or 1
            "triggerClick": 0,  # 0 or 1
        }

        self.poseControllerL = {
            # location in meters and orientation in quaternion
            "x": 0.5,  # +(x) is right in meters
            "y": 1.1,  # +(y) is up in meters
            "z": -1,  # -(z) is forward in meters
            "rW": 1,  # from 0 to 1
            "rX": 0,  # from -1 to 1
            "rY": 0,  # from -1 to 1
            "rZ": 0,  # from -1 to 1
            # velocity in meters/second
            "velX": 0,  # +(x) is right in meters/second
            "velY": 0,  # +(y) is right in meters/second
            "velZ": 0,  # -(z) is right in meters/second
            # Angular velocity of the pose in axis-angle
            # representation. The direction is the angle of
            # rotation and the magnitude is the angle around
            # that axis in radians/second.
            "angVelX": 0,
            "angVelY": 0,
            "angVelZ": 0,
            # inputs
            "grip": 0,  # 0 or 1
            "system": 0,  # 0 or 1
            "menu": 0,  # 0 or 1
            "trackpadClick": 0,  # 0 or 1
            "triggerValue": 0,  # from 0 to 1
            "trackpadX": 0,  # from -1 to 1
            "trackpadY": 0,  # from -1 to 1
            "trackpadTouch": 0,  # 0 or 1
            "triggerClick": 0,  # 0 or 1
        }

        self.incomingData_readonly = ""  # used to access recently read data from the server

        self._send = True
        self._recv = True
        self._exampleThread = True  # keep alive for the example thread

        self._recvDelay = 1 / 1000  # frequency at which other messages from the server are received
        self._sendDelay = (
            1 / 60
        )  # frequency at which the commands are sent out to the driver, default is 60 fps
        self._exampleThreadDelay = 1 / 50  # delay for the example thread, the delay is in seconds

        self.addr = addr
        self.port = port

    async def _socketInit(self):
        # not a thread

        # connect to the server
        self.reader, self.writer = await asyncio.open_connection(
            self.addr, self.port, loop=asyncio.get_event_loop()
        )
        # send poser id message
        self.writer.write(u.format_str_for_write("poser here"))

    async def close(self):
        # wait to stop thread
        while 1:  # a waiting loop
            await asyncio.sleep(2)
            try:
                if keyboard.is_pressed("q"):  # hold 'q' for 2 seconds to stop the poser
                    break

            except:
                break

        print("closing...")
        # kill all threads
        self._send = False
        self._recv = False
        self._exampleThread = False

        await asyncio.sleep(1)

        # disconnect from the server
        self.writer.write(u.format_str_for_write("CLOSE"))
        self.writer.close()

    async def send(self):
        # send thread
        while self._send:
            try:
                msg = u.format_str_for_write(
                    " ".join(
                        [str(i) for _, i in self.pose.items()]
                        + [str(i) for _, i in self.poseControllerR.items()]
                        + [str(i) for _, i in self.poseControllerL.items()]
                    )
                )

                self.writer.write(msg)

                await asyncio.sleep(self._sendDelay)

            except:
                # kill the thread if it breaks
                self._send = False
                break
        print(f"{self.send.__name__} stop")

    async def recv(self):
        # receive thread
        while self._recv:
            try:
                data = await u.read(self.reader)
                self.incomingData_readonly = data
                print([self.incomingData_readonly])

                await asyncio.sleep(self._recvDelay)

            except:
                # kill the thread if it breaks
                self._recv = False
                break
        print(f"{self.recv.__name__} stop")

    async def exampleThread(self):
        # example thread
        while self._exampleThread:
            try:

                # do some work here

                await asyncio.sleep(
                    self._exampleThreadDelay
                )  # thread sleep, every thread needs to have sleep in its loop to allow other threads to execute

            except:
                self._exampleThread = False
                break
        print(f"{self.exampleThread.__name__} stop")

    async def main(self):
        await self._socketInit()

        await asyncio.gather(
            self.send(),  # don't modify this
            self.recv(),  # don't modify this
            self.close(),  # don't modify this
            self.exampleThread(),  # example thread
        )


t = Poser()

asyncio.run(t.main())  # runs the threads in async mode
