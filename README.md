### Disclaimer
**This project is used for demonstration purposes only. Dissemination or reproduction of this project is strictly forbidden unless prior permission is obtained.**

# XerosKernel

* A simple kernel built on Bochs IA-32 emulator, capable of memory management, process creation, context switching, process scheduling, interprocess communication and hardware device interactions.

#### Specific kernel function links

* [Memory management](https://github.com/HollyZero/XerosKernel/blob/master/c/mem.c) 
* [Process Creation](https://github.com/HollyZero/XerosKernel/blob/master/c/create.c)
* [Context Switch](https://github.com/HollyZero/XerosKernel/blob/master/c/ctsw.c)
* [Process Scheduling - with pre-emption](https://github.com/HollyZero/XerosKernel/blob/master/c/disp.c) 
* [Interprocess Communication using direct messaging](https://github.com/HollyZero/XerosKernel/blob/master/c/msg.c)
* [Signaling](https://github.com/HollyZero/XerosKernel/blob/master/c/signal.c)
* [Hardware Device Interaction](https://github.com/HollyZero/XerosKernel/blob/master/c/di_calls.c)
* [Sleeping](https://github.com/HollyZero/XerosKernel/blob/master/c/sleep.c)

#### Running the project

* To build a Xeros image, type make in this directory. If you want to
launch bochs immediately after the make completes (assuming it is
successful) type "make beros" instead. This command will first build a
Xeros image and then start the bochs emulator.  (You can also go to
the c directory, where the source code you are editing resides, and
run the same make commands there.)

* When you run "make", or "make xeros", the first two steps below are
performed. If you run "make beros" then all 3 steps are done.

1. Change to the compile directory and run make

2. If step 1 succeeds, a boot image is built by changing to the boot
directory and running make there. 

3. If step 3 succeeds, bochs is run.

Once bochs is running, choose option 6 to start the simulation and to
load and run the created image.
