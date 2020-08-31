from unittest import mock

import numpy as np
from tests.data import rgby_tracking_image_np
from tests.mocks.mock_displayarray import MockReadUpdates
from virtualreality.util.utilz import BlobTracker


def test_init():
    with MockReadUpdates(0, [rgby_tracking_image_np]):
        with mock.patch("virtualreality.utilz.time.sleep"):
            b = BlobTracker(
                color_masks={
                    "tracker1": {"h": (98, 10), "s": (200, 55), "v": (250, 32)}
                }
            )

            assert b.camera_focal_length == 490
            assert b.BALL_RADIUS_CM == 2
            assert b.offsets == [0, 0, 0]
            assert b.cam_index == 0

            assert b.can_track

            assert b.frame_height == 358
            assert b.frame_width == 638

            assert b.markerMasks == {
                "tracker1": {"h": (98, 10), "s": (200, 55), "v": (250, 32)}
            }

            assert b.poses == {"tracker1": {"x": 0, "y": 0, "z": 0}}
            assert b.blobs == {"tracker1": None}


def test_find_blobs_in_frame():
    with MockReadUpdates(0, [rgby_tracking_image_np]):
        with mock.patch("virtualreality.utilz.time.sleep"):
            b = BlobTracker(
                color_masks={
                    "tracker1": {"h": (98, 10), "s": (200, 55), "v": (250, 32)}
                }
            )

            b.find_blobs_in_frame()

            assert (
                b.blobs["tracker1"]
                == np.asarray(
                    [
                        [[196, 272]],
                        [[194, 274]],
                        [[193, 274]],
                        [[192, 275]],
                        [[192, 276]],
                        [[191, 277]],
                        [[191, 282]],
                        [[192, 283]],
                        [[192, 284]],
                        [[195, 287]],
                        [[197, 287]],
                        [[198, 288]],
                        [[199, 288]],
                        [[200, 287]],
                        [[203, 287]],
                        [[205, 285]],
                        [[205, 284]],
                        [[206, 283]],
                        [[206, 278]],
                        [[205, 277]],
                        [[205, 276]],
                        [[203, 274]],
                        [[201, 274]],
                        [[200, 273]],
                        [[197, 273]],
                    ],
                    dtype=np.int32,
                )
            ).all()


def test_solve_blob_poses():
    with MockReadUpdates(0, [rgby_tracking_image_np]):
        with mock.patch("virtualreality.utilz.time.sleep"):
            b = BlobTracker(
                color_masks={
                    "tracker1": {"h": (98, 10), "s": (200, 55), "v": (250, 32)}
                }
            )

            b.find_blobs_in_frame()
            b.solve_blob_poses()

            assert b.poses == {
                "tracker1": {
                    "x": -0.34523609690742246,
                    "y": -0.28970498418772483,
                    "z": 1.4035486226748464,
                }
            }

            b.solve_blob_poses()

            assert b.poses == {
                "tracker1": {
                    "x": -0.6575314351920128,
                    "y": 0.11647778625690625,
                    "z": 1.466077432585446,
                }
            }
