from unittest import mock


class MockReadUpdates(object):
    def __init__(self, cam, frames, prepend=""):
        self.cam = cam
        self.frames = frames
        self.patches = []
        self.mocks = []
        self.prepend = prepend

    def __enter__(self):
        self.patches.append(
            mock.patch(self.prepend + "displayarray.window.subscriber_windows.display")
        )

        for m in self.patches:
            self.mocks.append(m.__enter__())

        main = self.mocks[0].return_value
        main.frames = dict()
        main.frames[str(self.cam)] = self.frames

    def __exit__(self, *args):
        for m in self.patches:
            m.__exit__(*args)

        self.patches = []
