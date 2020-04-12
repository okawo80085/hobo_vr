# hobo vr

*a pc vr solution for hobos* don't ask why

# docs
[documentation](https://github.com/okawo80085/hobo_vr/wiki)

# requirements
requirements c++(only if you are compiling it yourself):
```
openvr
```

requirements python:
```python
keyboard
opencv-python
numpy
pyserial # optional
imutils # optional
pykalman
```

# rambling/how it works

how it works:
```python
com_server.py # the server, starts a local server on port 6969
poserTemplate.py # poser(optional, you can write it all from scratch if you want to), sends pose data through the server, can be also used for tracking, refer to poseTracker.py for more examples

sample/ # steamvr driver, you need to register it's path with vrpathreg.exe
```

the steamvr driver connects to the server and gets pose data from it, the rest is handled by steamvr

![network_diagram](/images/network_diagram.jpg)

# TODO
on the steamvr/openvr side of things:
```
1. since the driver started as a heavily modified null driver, it lacks/unfinished on the display side of things, plus i don't have a spare display to experiment on
2. fix compositor on the driver side
```

## bug reporting/contributions
*just do it*
