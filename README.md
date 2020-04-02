# hobo vr

*a pc vr solution for hobos*

# installation/requirements

steamvr/openvr driver installation:
```
1. install steamvr though steam
2. go to "{steamvr installation directory}\bin\win64\"
3. open the console and run: .\vrpathreg.exe adddriver {full path to "sample/" directory}
```

requirements c++(only if you are compiling it yourself):
```
openvr
```

requirements python:
```python
keyboard # optional
opencv-python
numpy
pyserial # optional
imutils # optional
pykalman
```

# rambling/how it works

how it works:
```python
com_server.py # the server
poserTemplate.py # poser(optional, you can write it all from scratch if you want to), sends pose data through the server, can be also used for tracking, refer to poseTracker.py for more examples

sample/ # steamvr driver, you need to register it's path with vrpathreg.exe
```

the steamvr driver connects to the server and gets pose data from it, the rest is handled by steamvr


and since the poser it self it optional, here is a few thing about the message structure it has to follow:
```
1. poser needs to send "poser here\n"(encoded as a utf-8 binary string) as it first message to the server

2. poser needs to follow this message structure:

"x y z rW rX rY rZ x y z rW rX rY rZ grip system menu trackpadClick triggerValue trackpadX trackpadY trackpadTouch triggerClick x y z rW rX rY rZ grip system menu trackpadClick triggerValue trackpadX trackpadY trackpadTouch triggerClick"

where first 'x y z rW rX rY rZ' are the tracing data for the headset, in which 'x y z' are positional coordinates in meters and 'rW rX rY rZ' is rotations in space expressed as a quaternion

after that there is tracking data for one of the controllers in the same form as the headset one, after that come values for that controller's inputs(buttons, triggers, joysticks, etc.)

and after that is tracking and input values for the other controller, in the same form as the first one

3. all messages sent out by the poser need to be numeric only and of length 39(counting the vars)

4. also:
'grip' is a bool expressed as a number(0 is false, 1 is true)

'system' is a bool expressed as a number(0 is false, 1 is true)

'menu' is a bool expressed as a number(0 is false, 1 is true)

'trackpadClick' is a bool expressed as a number(0 is false, 1 is true)

'trackpadTouch' is a bool expressed as a number(0 is false, 1 is true)

'triggerClick' is a bool expressed as a number(0 is false, 1 is true)

'triggerValue' is a float with values from 0 to 1(needs to stay in that value range, because the steamvr driver expects that)

'trackpadX' is a float with values from -1 to 1(needs to stay in that value range, because the steamvr driver expects that)

'trackpadY' is a float with values from -1 to 1(needs to stay in that value range, because the steamvr driver expects that)

also changes in 'trackpadX'/'trackpadY' are useless if 'trackpadTouch' is not set to true
```
## poserTemplate

or, if you are ok with python, you can modify `poserTemplate.py` for your needs and save yourself the headache of starting from scratch. you can also use `poseTracker.py` as a reference of how to manage tracking, don't rely on it too much though, because it is made for my hardware.

# TODO

on the steamvr/openvr side of things:
```
1. since the driver started as a heavily modified null driver, it lacks/unfinished on the display side of things, plus i don't have a spare display to experiment on
2. that's it really, for now at least
```
