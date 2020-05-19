from unittest import mock

from displayarray import read_updates
from tests.data import rgby_tracking_image_np
from tests.mocks.mock_displayarray import MockReadUpdates


def test_mocks_read():
    with MockReadUpdates(0, [rgby_tracking_image_np]):
        a = read_updates()
        assert isinstance(a, mock.MagicMock)
        a.update()
        assert (a.frames["0"][0] == rgby_tracking_image_np).all()
