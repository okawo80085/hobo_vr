# linux installers

* `offline/` contains the offline installer `.deb` package and the installer builder
* `online/` contains the online installer `.deb` package and the installer builder, this installer needs an internet connection, it will install the latest driver from master

both installers have their own `build_deb.sh` scripts, those are the installer builders, only use them to build the `.deb` packages

# usage
to install the deb package, either use your package manager or run this command
```bash
sudo dpkg -i hobo_vr.deb

# if you have any missing dependencies also run this:
sudo apt --fix-broken install

```

to uninstall the deb package, either again use your package manager or run this command
```bash
sudo dpkg -r hobovr
```

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
