import multiprocessing
import pathlib
import sys
from typing import Union

import librosa
import numpy as np
import pyaudio

ourpath = pathlib.Path(__file__).parent


class InputConfig(object):
    min_seconds = 2
    sampling_rate = 44100
    stream_chunk_size = 1024
    input_channels = 1


easy_music = next(ourpath.joinpath("easy_music").glob("*.mp3"))
easy_bpm = int(open(str(next(ourpath.joinpath("easy_music").glob("*.txt")))).read())
normal_music = next(ourpath.joinpath("normal_music").glob("*.mp3"))
normal_bpm = int(open(str(next(ourpath.joinpath("normal_music").glob("*.txt")))).read())
hard_music = next(ourpath.joinpath("hard_music").glob("*.mp3"))
hard_bpm = int(open(str(next(ourpath.joinpath("hard_music").glob("*.txt")))).read())
menu_music = next(ourpath.joinpath("menu_music").glob("*.mp3"))
menu_bpm = int(open(str(next(ourpath.joinpath("menu_music").glob("*.txt")))).read())


def read_audio_file(file_path: Union[str, pathlib.Path], config: InputConfig):
    """
    Reads in an audio file and converts to a numpy array.
    >>> f = read_audio_file('./sw.mp3', InputConfig())
    >>> print(f)
    [[-1.2063791e-04 -1.0168436e-04 -3.2342479e-05 ... -7.7938515e-05
      -8.8288434e-05 -5.1688534e-05]
     [ 9.4297948e-06 -2.1362750e-05 -7.0384616e-05 ...  3.9314014e-05
       5.7227458e-05  3.8280330e-05]]

    """
    if not isinstance(file_path, str):
        file_path = str(file_path)
    min_samples = int(config.min_seconds * config.sampling_rate)
    try:
        audio_time_series, sampling_rate = librosa.load(
            file_path, sr=config.sampling_rate, mono=False
        )
        trimmed_audio, trim_indexes = librosa.effects.trim(audio_time_series)

        if trimmed_audio.shape[-1] < min_samples:
            padding = min_samples - len(trimmed_audio)
            offset = padding // 2
            trimmed_audio = np.pad(
                trimmed_audio, (offset, padding - offset), "constant"
            )
        return trimmed_audio
    except Exception as e:
        print(f"Exception while reading {file_path}: {e}", sys.stderr)
        return np.zeros(min_samples, dtype=np.float32)


def play_audio(array: np.ndarray, config: InputConfig, loop=False):
    """
    Plays a numpy array as audio.

    from: https://www.scivision.dev/playing-sounds-from-numpy-arrays-in-python/

    >>> i = InputConfig()
    >>> f = read_audio_file('./sw.mp3', i)
    >>> play_audio(f, i)

    # song: https://www.youtube.com/watch?v=d-nxW9qBtxQ

    """
    if len(array.shape) == 2:
        channels = int(array.shape[0])
    else:
        channels = 1

    array = np.transpose(array)
    p = pyaudio.PyAudio()
    stream = p.open(
        rate=config.sampling_rate,
        format=pyaudio.paFloat32,
        channels=channels,
        output=True,
    )
    arrby = array.tobytes()
    stream.write(arrby)
    while loop:
        stream.write(arrby)
    stream.close()
    p.terminate()


def get_audio_thread(music: Union[str, pathlib.Path], loop=False):
    audio_cfg = InputConfig()
    np_file = read_audio_file(music, audio_cfg)
    audio_thread = multiprocessing.Process(
        target=play_audio, args=(np_file, audio_cfg, loop)
    )
    return audio_thread
