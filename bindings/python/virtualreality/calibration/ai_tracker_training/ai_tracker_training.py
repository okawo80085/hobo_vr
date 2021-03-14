import math as m
import pathlib
import pickle
import time
from typing import Optional, Union

import cv2
import numpy as np

try:
    from displayarray import read_updates
    from displayarray.input import mouse_loop
    from displayarray.window import SubscriberWindows
    from displayarray.effects.overlay import overlay_transparent
    from displayarray.effects.transform import transform_about_center

except Exception as e:
    print(f"failed to import displayarray, this module will not be available\n\n")
    raise e

from virtualreality.calibration.ai_tracker_training.audio import (
    get_audio_thread,
    menu_music,
    easy_music,
    normal_music,
    hard_music,
    easy_bpm,
    normal_bpm,
    hard_bpm,
)
from virtualreality.calibration.ai_tracker_training.video import menu_np
from virtualreality.util.IMU import get_coms_in_range, get_i2c_imu, IMU

ourpath = pathlib.Path(__file__).parent


def get_heartbeat_callback(bpm, sharpness=10):
    def heartbeat_callback(t):
        tempo = bpm / 60.0
        val = m.cos(m.pi * t * tempo) ** (2 * sharpness)
        return val

    return heartbeat_callback


class GameState:
    def __init__(self, cam, imu, out_name, music_file=None, loop=False):
        self.cam = cam
        self.imu = imu
        self.out_name = out_name
        self.music_file = music_file
        self.next_state = None
        self.video_shape = None
        self.mouse_loop = mouse_loop(self.on_mouse)
        self.loop = loop

    def run(self) -> Optional["GameState"]:
        audio_thread = None
        if self.music_file is not None:
            audio_thread = get_audio_thread(self.music_file, self.loop)
        if not isinstance(self.cam, SubscriberWindows):
            self.cam = read_updates(
                self.cam,
                size=(-1, -1),
            )
        started = False
        f_prev = None
        for f in self.cam:
            if f and f_prev is not next(iter(f.values()))[0]:
                vid: np.ndarray = next(iter(f.values()))[0]
                if not started:
                    if audio_thread is not None:
                        audio_thread.start()
                    self.on_start()
                    started = True
                if self.next_state is not None:
                    audio_thread.terminate()
                    return self.next_state
                self.video_shape = vid.shape
                vid = self.preprocess_video(vid)
                a = [self.on_video(vid)]
                self.cam.display_frames(a)
                f_prev = vid
                audio_thread.join(0)
                if not audio_thread.is_alive():
                    self.next_state = self.on_end()
                    audio_thread.terminate()
                    return self.next_state
        audio_thread.terminate()

    def on_mouse(self, mouse_state):
        pass

    def preprocess_video(self, video: np.ndarray) -> np.ndarray:
        return np.flip(np.copy(video), 1)

    def on_video(self, video: np.ndarray) -> np.ndarray:
        return video

    def on_start(self):
        pass

    def on_end(self) -> Optional["GameState"]:
        return MenuState(self.cam, self.imu, self.out_name)


class AIRecordingState(GameState):
    class GoalPose(object):
        x: float = 0
        y: float = 0
        z: float = 1
        roll: float = 0
        pitch: float = 0
        yaw: float = 0

        def __eq__(self, other):
            if isinstance(other, AIRecordingState.GoalPose):
                out = (
                    self.x == other.x
                    and self.y == other.y
                    and self.z == other.z
                    and self.roll == other.roll
                    and self.pitch == other.pitch
                    and self.yaw == other.yaw
                )
                return out

        def __mul__(self, other):
            out_pose = AIRecordingState.GoalPose()
            if isinstance(other, AIRecordingState.GoalPose):
                out_pose.x = self.x * other.x
                out_pose.y = self.y * other.y
                out_pose.z = self.z * other.z
                out_pose.roll = self.roll * other.roll
                out_pose.pitch = self.pitch * other.pitch
                out_pose.yaw = self.yaw * other.yaw
            else:
                out_pose.x = self.x * other
                out_pose.y = self.y * other
                out_pose.z = self.z * other
                out_pose.roll = self.roll * other
                out_pose.pitch = self.pitch * other
                out_pose.yaw = self.yaw * other
            return out_pose

        def __add__(self, other):
            out_pose = AIRecordingState.GoalPose()
            if isinstance(other, AIRecordingState.GoalPose):
                out_pose.x = self.x + other.x
                out_pose.y = self.y + other.y
                out_pose.z = self.z + other.z
                out_pose.roll = self.roll + other.roll
                out_pose.pitch = self.pitch + other.pitch
                out_pose.yaw = self.yaw + other.yaw
            else:
                out_pose.x = self.x + other
                out_pose.y = self.y + other
                out_pose.z = self.z + other
                out_pose.roll = self.roll + other
                out_pose.pitch = self.pitch + other
                out_pose.yaw = self.yaw + other
            return out_pose

    class Record(list):
        def __init__(self, grabbed_frame):
            super().__init__()
            self.grabbed_frame = grabbed_frame

    def __init__(self, cam, grabbed_frame, imu, out_name, music, bpm):
        super().__init__(cam, imu, out_name, music)
        self.heartbeat = get_heartbeat_callback(bpm)
        self.bpm = bpm / 2.0
        self.grabbed_frame = grabbed_frame
        self.t0 = None
        self.prev_beat_distance = 0
        self.goal_pose = AIRecordingState.GoalPose()
        self.prev_goal = AIRecordingState.GoalPose()
        hsv = cv2.cvtColor(self.grabbed_frame, cv2.COLOR_BGR2HSV)
        self.grabbed_frame = np.append(self.grabbed_frame, hsv[:, :, 2:], -1)
        self.goal_square = np.zeros((100, 200, 4))
        self.goal_square[:, :, 3] = 255
        self.goal_square[:, :, 1] = 255
        self.goal_square = overlay_transparent(
            np.zeros((self.grabbed_frame.shape[0], self.grabbed_frame.shape[1], 3)),
            self.goal_square,
        )
        hsv_square = cv2.cvtColor(self.goal_square.astype(np.uint8), cv2.COLOR_BGR2HSV)
        self.goal_square = np.append(self.goal_square, hsv_square[:, :, 2:] * 0.5, -1)
        self.out_video = cv2.VideoWriter(
            f"{out_name}.avi",
            cv2.VideoWriter_fourcc(*"MJPG"),
            20.0,
            (grabbed_frame.shape[1], grabbed_frame.shape[0]),
            True,
        )
        self.out_record = AIRecordingState.Record(self.grabbed_frame)
        self.out_name = out_name

    def on_start(self):
        self.t0 = time.time()

    def make_new_goal(self):
        g = AIRecordingState.GoalPose()
        g.x = (np.random.random_sample() - 0.5) * self.video_shape[0]
        g.y = (np.random.random_sample() - 0.5) * self.video_shape[1]
        g.z = (np.random.random_sample() + 0.25) * 2
        g.roll = (np.random.random_sample() - 0.5) * 90
        g.yaw = (np.random.random_sample() - 0.5) * 120
        g.pitch = (np.random.random_sample() - 0.5) * 120
        self.goal_pose = g

    def preprocess_video(self, video: np.ndarray) -> np.ndarray:
        self.out_video.write(video)
        return super().preprocess_video(video)

    def on_video(self, video: np.ndarray) -> np.ndarray:
        current_time = time.time()
        wall_time = current_time - self.t0
        spb = 60.0 / self.bpm
        wall_beat_distance = (wall_time % spb) / spb
        beat = self.heartbeat(wall_time) / 2.0
        a = (1 - beat) * video + beat * 255
        if wall_beat_distance < self.prev_beat_distance:
            zero_goal = AIRecordingState.GoalPose()
            self.prev_goal = self.goal_pose
            if self.prev_goal == zero_goal:
                self.make_new_goal()
            else:
                self.goal_pose = zero_goal
        beat_distance = (np.tanh(wall_beat_distance * np.pi * 2.0 - np.pi) / 2.0) + 0.5
        g = self.prev_goal * (1 - beat_distance) + self.goal_pose * beat_distance
        xfromed_grab = transform_about_center(
            self.grabbed_frame,
            scale_multiplier=(g.z, g.z),
            translation=(g.x, g.y),
            rotation_degrees=g.roll,
            skew=(g.yaw, g.pitch),
        )
        xformed_goal = transform_about_center(
            self.goal_square,
            scale_multiplier=(self.goal_pose.z, self.goal_pose.z),
            translation=(self.goal_pose.x, self.goal_pose.y),
            rotation_degrees=self.goal_pose.roll,
            skew=(self.goal_pose.yaw, self.goal_pose.pitch),
        )
        a = overlay_transparent(a, xformed_goal)
        a = overlay_transparent(a, xfromed_grab)
        self.prev_beat_distance = wall_beat_distance
        q = self.imu.protocol.imu.get_orientation(0.1, 0.1, 0.1, 0.1)
        acc = self.imu.protocol.imu.get_acc(q)
        self.out_record.append((g, wall_time, q, acc))

        return a.astype(np.uint8)

    def on_end(self) -> Optional["GameState"]:
        with open(f"{self.out_name}.pickle", "wb") as out_pickle:
            pickle.dump(self.out_record, out_pickle)
        self.out_video.release()
        return super().on_end()


class EasyState(AIRecordingState):
    def __init__(self, cam, grabbed_frame, imu, out_name):
        super().__init__(cam, grabbed_frame, imu, out_name, easy_music, easy_bpm / 4)


class NormalState(AIRecordingState):
    def __init__(self, cam, grabbed_frame, imu, out_name):
        super().__init__(
            cam, grabbed_frame, imu, out_name, normal_music, normal_bpm / 2
        )


class HardState(AIRecordingState):
    def __init__(self, cam, grabbed_frame, imu, out_name):
        super().__init__(cam, grabbed_frame, imu, out_name, hard_music, hard_bpm)


class MenuState(GameState):
    def __init__(self, cam, imu, out_name):
        super().__init__(cam, imu, out_name, menu_music, loop=True)
        self.last_frame = None
        self.imu_failure = None
        self.menu_np = menu_np
        self.out_name = out_name
        try:
            if isinstance(imu, IMU):
                self.pro = imu
            if imu is not None and imu is not False:
                if imu is True:
                    com = get_coms_in_range()
                    if len(com) == 0:
                        raise RuntimeError("No COMs found.")
                    com = com[0]
                else:
                    com = imu

                con, self.pro = get_i2c_imu(com)
                self.pro.start()
        except Exception as e:
            self.imu_failure = e
            self.menu_np = cv2.putText(
                self.menu_np,
                f"IMU Failed to initialize:  {e}",
                (0, 24),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.5,
                (0, 0, 255, 255),
                1,
            )

    def preprocess_video(self, video: np.ndarray) -> np.ndarray:
        self.last_frame = video
        return super().preprocess_video(video)

    def on_video(self, video):
        a = overlay_transparent(video, self.menu_np)
        return a

    def on_mouse(self, mouse_event):
        overlay_button_positions = {
            "easy": [[64, 128], [64 + 128, 128 + 128]],
            "normal": [[64 * 4, 64 * 2], [64 * 4 + 128, 64 * 2 + 128]],
            "hard": [[64 * 7, 64 * 2], [64 * 7 + 128, 64 * 2 + 128]],
        }
        button_commands = {
            "easy": EasyState,
            "normal": NormalState,
            "hard": HardState,
        }

        if (
            self.video_shape is not None
            and mouse_event is not None
            and mouse_event.flags == cv2.EVENT_FLAG_LBUTTON
        ):
            h, w = menu_np.shape[0], menu_np.shape[1]
            v_h, v_w = self.video_shape[0], self.video_shape[1]

            top_add = (v_h - h) / 2.0
            left_add = (v_w - w) / 2.0

            for key, value in overlay_button_positions.items():
                if (
                    value[0][0] < mouse_event.x - left_add < value[1][0]
                    and value[0][1] < mouse_event.y - top_add < value[1][1]
                ):
                    self.next_state = button_commands[key](
                        self.cam, np.flip(self.last_frame, 1), self.pro, self.out_name
                    )


def run_game(
    start_state,
    led_tracker=True,
    imu: Optional[Union[str, bool]] = True,
    out_name: str = "train",
):
    from virtualreality.calibration.manual_color_mask_calibration import (
        list_supported_capture_properties,
    )
    import logging

    cam = 0
    vs = cv2.VideoCapture(cam)
    vs.set(cv2.CAP_PROP_EXPOSURE, -7)
    vs_supported = list_supported_capture_properties(vs)
    if led_tracker:
        if "CAP_PROP_AUTO_EXPOSURE" not in vs_supported:
            logging.warning(
                f"Camera {cam} does not support turning on/off auto exposure."
            )
        else:
            vs.set(cv2.CAP_PROP_AUTO_EXPOSURE, 0.25)

        if "CAP_PROP_EXPOSURE" not in vs_supported:
            logging.warning(f"Camera {cam} does not support directly setting exposure.")
        else:
            vs.set(cv2.CAP_PROP_EXPOSURE, -7)
    else:
        if "CAP_PROP_AUTO_EXPOSURE" not in vs_supported:
            logging.warning(
                f"Camera {cam} does not support turning on/off auto exposure."
            )
        else:
            vs.set(cv2.CAP_PROP_AUTO_EXPOSURE, 100.0)

        vs.release()

    current_state = start_state(0, imu, out_name)
    while current_state is not None:
        current_state = current_state.run()


if __name__ == "__main__":
    run_game(MenuState)
