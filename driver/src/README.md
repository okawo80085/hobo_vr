
## linux

### compile
to compile the driver on linux use this
```
mkdir ./build
cd ./build
cmake .. -DCMAKE_PREFIX_PATH=/opt/Qt/5.6/gcc_64/lib/cmake -DCMAKE_BUILD_TYPE=Release
make -j4
mv ./libdriver_hobovr.so ./driver_hobovr.so
```

### install compiled
then copy `driver_hobovr.so` to `hobovr/bin/linux32` and `hobovr/bin/linux64`

## windows

### compile
just use the `driver_hobovr.vcxproj` file, so far its the only thing that works

### install compiled
move the the 32 bit dll into `hobovr/bin/win32` and 64 bit dll into `hobovr/bin/win64`