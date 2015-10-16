*** Danger Danger Danger ***

The iOS build of VTK is in an alpha state. Please do try it out but understand
it is very new.

To build a VTK application for iOS you will need a Mac/Macbook that has Xcode on
it and the iOS dev kit. What follows are the basic steps:


* Make a directory for the host binaries (e.g. vtkbin). I typically create this
* next to the VTK source tree.

create an install directory in vtkbin ala cd vtkbin; mkdir install
Run cmake on that directory with -DVTK_IOS_BUILD=ON
configure and generate as usual

* Choose an example Xcode project in Examples/iOS. If you modified the framework
* install path you will need to repair the framework reference, the framework
* search path and the header search path.

* If you built VTK to run on the actual device, make sure you have all
* provisioning certificates and devices setup properly. Apple requires iOS
* developers to have Apple-issued certificates and their devices have to be
* registered before development apps may be run on the device.

* Choose the correct target device (i.e., device or simulator) then build and
* run the Xcode project.

* Enjoy a delicious mudslide, you earned it!

The GLPaint example is a modified version of Apple's GLPaint example. So credit
to them for the original example file and framework.

The VolumeRender example requires OpenGL ES3 so it will only work on
devices that support ES3, which I believe includes iphone 5s or later and
ipad Air or later