# hobo vr

*a pc vr solution for hobos*

steamvr/openvr driver installation:
```
1. install steamvr though steam
2. go to "{steamvr installation directory}\bin\win64\"
3. open a console and run: .\vrpathreg.exe adddriver {full path to "sample/" directory}
```

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
poseTracker.py # poser(optional), sends pose data through the server

sample/ # steamvr driver, you need to register it's path with vrpathreg
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
