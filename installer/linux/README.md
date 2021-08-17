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
