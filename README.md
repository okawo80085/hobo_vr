# hobo vr

*a pc vr solution for hobos*

requirements c++:
```
openvr
```

requirements python:
```
keyboard
opencv-python
numpy
pyserial
imutils
```

how it works:
```python
com_server.py # the server
poseTracker.py # poser, sends pose data through the server

sample/ # steamvr driver, you need to register it's path with vrpathreg
```

the steamvr driver connects to a python server and gets pose data from it, the rest is handled by steamvr