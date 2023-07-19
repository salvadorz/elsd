## Internal Error `Oops`

This is the output  after try to write to `faulty` Kernel Module

```shell
root@qemuarm64:~# echo "Hello machine" > /dev/faulty 
[   70.748980] Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
[   70.749430] Mem abort info:
[   70.749520]   ESR = 0x0000000096000045
[   70.749640]   EC = 0x25: DABT (current EL), IL = 32 bits
[   70.749780]   SET = 0, FnV = 0
[   70.749866]   EA = 0, S1PTW = 0
[   70.749955]   FSC = 0x05: level 1 translation fault
[   70.750084] Data abort info:
[   70.750172]   ISV = 0, ISS = 0x00000045
[   70.750280]   CM = 0, WnR = 1
[   70.750479] user pgtable: 4k pages, 39-bit VAs, pgdp=0000000043727000
[   70.750624] [0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000
[   70.751009] Internal error: Oops: 96000045 [#1] PREEMPT SMP
[   70.751289] Modules linked in: scull(O) hello(O) faulty(O)
[   70.751783] CPU: 1 PID: 450 Comm: sh Tainted: G           O      5.15.113-yocto-standard #1
[   70.751946] Hardware name: linux,dummy-virt (DT)
[   70.752207] pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
[   70.752489] pc : faulty_write+0x18/0x20 [faulty]
[   70.752967] lr : vfs_write+0xf8/0x29c
[   70.753057] sp : ffffffc00ad7bd80
[   70.753147] x29: ffffffc00ad7bd80 x28: ffffff800201d280 x27: 0000000000000000
[   70.753378] x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
[   70.753562] x23: 0000000000000000 x22: ffffffc00ad7bdf0 x21: 000000558ce85ba0
[   70.753747] x20: ffffff8003723e00 x19: 000000000000000e x18: 0000000000000000
[   70.753931] x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
[   70.754115] x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
[   70.754298] x11: 0000000000000000 x10: 0000000000000000 x9 : ffffffc00826541c
[   70.754510] x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
[   70.754693] x5 : 0000000000000001 x4 : ffffffc000b60000 x3 : ffffffc00ad7bdf0
[   70.754874] x2 : 000000000000000e x1 : 0000000000000000 x0 : 0000000000000000
[   70.755190] Call trace:
[   70.755290]  faulty_write+0x18/0x20 [faulty]
[   70.755438]  ksys_write+0x70/0x100
[   70.755528]  __arm64_sys_write+0x24/0x30
[   70.755629]  invoke_syscall+0x5c/0x130
[   70.755738]  el0_svc_common.constprop.0+0x4c/0x100
[   70.755860]  do_el0_svc+0x4c/0xb4
[   70.755949]  el0_svc+0x28/0x80
[   70.756036]  el0t_64_sync_handler+0xa4/0x130
[   70.756147]  el0t_64_sync+0x1a0/0x1a4
[   70.756441] Code: d2800001 d2800000 d503233f d50323bf (b900003f) 
[   70.756728] ---[ end trace 9b9b50feae5defe7 ]---
Segmentation fault

```
### Analysis
 * `Unable to handle kernel NULL pointer dereference at virtual address 0` Tells what happened.
 * `Internal error: Oops: 96000045 [#1] PREEMPT SMP` Indicates the  type of error at kernel.
 * `Modules linked in: scull(O) hello(O) faulty(O)` Tells which modules where linked.
 * `CPU: 1 PID: 450 Comm: sh Tainted: G           O      5.15.113-yocto-standard #1`
   * Indicates in which CPU occured.
   * Mentions the `PID` process which cause the `Segmentation fault` 
   * `sh` is the command which was being executed.
   * `Tainted: G` says the Kernel was tainted  with not signed modules.
   * `O` means there is a Out-of-tree module
   * `5.15.113-yocto-standard #1` Says the Kernel version
 * `pc : faulty_write+0x18/0x20 [faulty]` Shows the program counter telling which instruction caused the violation. In this case the `faulty_write` function with an _offset_`0x18`
 * `lr : vfs_write+0xf8/0x29c` is the return address, which is the kernel function responsible to perform file writes.
 * `sp` shows the address of hte stack poointer at that moment.
 * `Call trace` shows the tracing of the calls when the error ocurred.

> This provides enough information to find the root cause of the error, and the responsible entity.