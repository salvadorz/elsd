## FAQS

### Issues loading Kernel Modules

If facing issues loading Kernel modules with `Required key not available` Is due `EFI_SECURE_BOOT_SIG_ENFORCE` kernel config has been enabled. That prevents from loading unsigned third party modules if _UEFI Secure Boot_ is enabled. Could be disable from the bios and running 
 ```shell 
sudo apt install mokutil
sudo mokutil --disable-validation
```
It will require to create a password. The password should be at least 8 characters long. After you reboot, UEFI will ask if you want to change security settings. Choose "Yes".

Then you will be asked to enter the previously created password. Some UEFI firmware asks not for the full password, but to enter some characters of it, like 1st, 3rd, etc.

