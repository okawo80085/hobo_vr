# Windows installer

* `hobovr_setup_web.exe` installs the latest hobo_vr driver from master. This installer needs an internet connection.
* `hobovr_installer.nsi` is the nsi code that compiles into hobovr_setup_web.exe.

# usage
to install the package, download the exe file and run it. If windows defender pops up, click more info, then run anyway. (exe certificates would fix this, but this exe isn't finalized)

There currently isn't an uninstall script yet. To uninstall, deleting the `HoboVR` folder under `C:\Program Files (x86)` should get rid of every installed file.
```
