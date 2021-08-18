# Windows installer

* `hobovr_setup_web.exe` installs the latest hobo_vr driver from master. This installer needs an internet connection.
* `hobovr_installer.nsi` is the nsi code that compiles into hobovr_setup_web.exe.

# usage
to install the package, download the exe file and run it. If windows defender pops up, click more info, then run anyway. (exe certificates would fix this, but this exe isn't finalized)

There currently isn't an uninstall script yet. To uninstall, deleting the `HoboVR` folder under `C:\Program Files (x86)` should get rid of every installed file.

# building the installer
Download NSIS from here: https://nsis.sourceforge.io/Download
Copy and paste the contents of `plugin.zip/Plugins` in the following plugins to `C:\Program Files (x86)\NSIS\Plugins`:
* Inetc: https://nsis.sourceforge.io/Inetc_plug-in
* Nsisunz: https://nsis.sourceforge.io/Nsisunz_plug-in
* ExecDos: https://nsis.sourceforge.io/ExecDos_plug-in
* Locate: https://nsis.sourceforge.io/Locate_plugin
* AccessControl: https://nsis.sourceforge.io/AccessControl_plug-in
For nsiunz and Locate, copy the dll under release to the plugins directory, and the x86 directories.
For AccessControl, move the contents of the i386 directories into the x86 directories

The compiler then expectes a LICENSE file in the directory above the nsi file. If cloned, this should work as is. Otherwise, the LICENSE file needs to be moved or the path in the nsi file changed.
