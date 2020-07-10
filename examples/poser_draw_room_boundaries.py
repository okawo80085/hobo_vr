'''

first positions both controllers to ROOM_CENTER then waits for resume

then draws a box starting at START_POS with STEP*STEP_MUL for NUM_ITER times for each element of APPLY_DIM

needs aioconsole module installed

'''



import asyncio
import time
import numpy as np
import pyrr

from virtualreality import templates
from virtualreality.server import server
from aioconsole import ainput

poser = templates.PoserClient()

START_POS = np.array([-0.124638+(3.3/2), -1.439744, 0.623368+1], dtype=np.float64) # [x, y, z]

ROOM_CENTER = np.array([-0.124638, -3.439744, 0.623368], dtype=np.float64) # [x, y, z]


STEP = 0.1
STEP_MUL = -1

NUM_ITER = 33

APPLY_DIM = np.array([
            [1, 0, 0],
            [0, 0, 1],
            [-1, 0, 0],
            [0, 0, -1],
            ], dtype=np.float64) # this dictates the shape of the boundary

np.set_printoptions(formatter={'all':lambda x:'{:+.4f}, '.format(x)})


@poser.thread_register(1/30)
async def example_thread():
    global START_POS

    poser.pose_controller_r.x = ROOM_CENTER[0]
    poser.pose_controller_r.y = ROOM_CENTER[1]
    poser.pose_controller_r.z = ROOM_CENTER[2]

    poser.pose_controller_l.x = ROOM_CENTER[0]
    poser.pose_controller_l.y = ROOM_CENTER[1]
    poser.pose_controller_l.z = ROOM_CENTER[2]

    await ainput('controllers at ROOM_CENTER, press enter to continue...')

    await asyncio.sleep(1)

    print ('starting boundary draw...')

    poser.pose_controller_r.trigger_value = 1
    poser.pose_controller_r.trigger_click = 1

    poser.pose_controller_l.trigger_value = 1
    poser.pose_controller_l.trigger_click = 1

    for i in APPLY_DIM:
        for _ in range(NUM_ITER):
            if not poser.coro_keep_alive["example_thread"][0]:
                break

            poser.pose.x = START_POS[0]
            poser.pose.y = START_POS[1]
            poser.pose.z = START_POS[2]

            poser.pose_controller_r.x = START_POS[0]
            poser.pose_controller_r.y = START_POS[1]
            poser.pose_controller_r.z = START_POS[2]

            poser.pose_controller_l.x = START_POS[0]
            poser.pose_controller_l.y = START_POS[1]
            poser.pose_controller_l.z = START_POS[2]

            START_POS += i * (STEP * STEP_MUL)

            print (START_POS)

            await asyncio.sleep(poser.coro_keep_alive["example_thread"][1])


    poser.pose_controller_r.trigger_value = 0
    poser.pose_controller_r.trigger_click = 0

    poser.pose_controller_l.trigger_value = 0
    poser.pose_controller_l.trigger_click = 0

    poser.coro_keep_alive["example_thread"][0] = False
    await asyncio.sleep(1/10)
    print ('drawing boundaries done')

    poser.coro_keep_alive["close"][0] = False # close poser

asyncio.run(poser.main())