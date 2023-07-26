## FAQS

### Issues loading Kernel Modules

If facing issues loading Kernel modules with `Required key not available` Is due `EFI_SECURE_BOOT_SIG_ENFORCE` kernel config has been enabled. That prevents from loading unsigned third party modules if _UEFI Secure Boot_ is enabled. Could be disable from the bios and running 
 ```shell 
sudo apt install mokutil
sudo mokutil --disable-validation
```
It will require to create a password. The password should be at least 8 characters long. After you reboot, UEFI will ask if you want to change security settings. Choose "Yes".

Then you will be asked to enter the previously created password. Some UEFI firmware asks not for the full password, but to enter some characters of it, like 1st, 3rd, etc.

### Compile and use a kernel in WSL2

```shell
sudo apt install build-essential flex bison libssl-dev libelf-dev git dwarves
git clone https://github.com/microsoft/WSL2-Linux-Kernel.git
cd WSL2-Linux-Kernel
cp Microsoft/config-wsl .config
make -j $(expr $(nproc) - 1)
```

 * Create a [local configuration](https://www.bleepingcomputer.com/news/microsoft/windows-10-wsl2-now-allows-you-to-configure-global-options/) by doing a copy `\\wsl$\<DISTRO>\home\<USER>\WSL2-Linux-Kernel\arch\x86\boot\bzimage` to your profile `%userprofile%\.wslconfig`
```
[wsl2]
kernel=C:\\Users\\<YOUR_USER>\\bzimage
```
 * In *PowerShell* `wsl --shutdown`

* `Makefile`
  ```Make

  obj-m:=lkm_example.o

  all:
    make -C $(shell pwd)/WSL2-Linux-Kernel M=$(shell pwd) modules
  clean:
    make -C $(shell pwd)/WSL2-Linux-Kernel M=$(shell pwd) clean
  ```

### Loading Kernel Modules on WSL
* `sudo -e /etc/modules-load.d/modules.conf` (or your preferred editor as sudo).
* Add the names of the kernel modules you would like to load, one line per module.
* Exit WSL
* `wsl --shutdown`
* Restart WSL

* More [Info](https://unix.stackexchange.com/questions/594470/wsl-2-does-not-have-lib-modules)
* Other [Info](https://gist.github.com/charlie-x/96a92aaaa04346bdf1fb4c3621f3e392)

