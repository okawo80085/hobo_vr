# hobo vr

*a pc vr solution for hobos* don't ask why

# docs
[documentation](https://github.com/okawo80085/hobo_vr/wiki)

# Install

## Driver Setup
[quick start guide](https://github.com/okawo80085/hobo_vr/wiki/quick-start-guide#initial-setup)

## Python Setup
```
> pip install git+https://github.com/okawo80085/hobo_vr
```

# requirements
requirements c++(only if you are compiling it yourself):
```
openvr
```

# rambling/how it works

how it works:
```python
python -m virtualreality.server # the server, more info: https://github.com/okawo80085/hobo_vr/wiki/server
examples/poserTemplate.py # a poser template, more info: https://github.com/okawo80085/hobo_vr/wiki/poser-script

sample/ # steamvr driver, mroe info: https://github.com/okawo80085/hobo_vr/wiki/driver
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
