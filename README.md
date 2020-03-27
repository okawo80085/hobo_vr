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

driver_sample # openvr driver
```

the openvr driver connects to a python server and gets pose data from it, the rest is handled by steamvr