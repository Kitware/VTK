The iOS build of VTK is in a beta state. Please do try it out but understand
it is very new.

To build a VTK application for iOS you will need a Mac/Macbook that has Xcode on
it and the iOS dev kit. What follows are the basic steps:

* Make a directory for the host binaries (e.g. vtkios). I typically create this
* next to the VTK source tree.

The end result of your VTK build will be a vtk framework that includes header
files and libraries for various architectures. You need to pick a place to
install the resulting framework. I usually create a directory named install
directory in vtkios ala cd vtkios; mkdir install

* Run cmake on vtkios with -DVTK_IOS_BUILD=ON, if you use the gui add a
* boolean entry with that name prior to configuring and set it on.

* Change the VTK_INSTALL_PREFIX to where you want the framework written.

* configure and generate as usual

* Once done build your framework using make or ninja for the build process.

Once the framework is built it will automatically be installed. Now you can
try building an iOS application that uses the framework.

* Start up XCode and load an example Xcode project from Examples/iOS. They will
* be located in

<yourBinDir>/CMakeExternals/Build/vtk-ios-device-armv7/Examples/iOS

* likely need to update a couple setting in the project to find the vtk fraework
* and its header files. You may also get unresolved link errors related to the
* c++ standard libaries. I have had to change the stdlib settings in XCode away
* from the compiler default to explicitly select a std c++ library

* If you built VTK to run on the actual device, make sure you have all
* provisioning certificates and devices setup properly. Apple requires iOS
* developers to have Apple-issued certificates and their devices have to be
* registered before development apps may be run on the device.

* Choose the correct target device (i.e., device or simulator) then build and
* run the Xcode project.

I typically test on a device as that is the end target and simulators can be a
bit twitchy sometimes.

The GLPaint example is a modified version of Apple's GLPaint example. So credit
to them for the original example file and framework.

The VolumeRender and PlaneView examples require OpenGL ES3 so it will only
work on devices that support ES3, which includes iphone 5s or later and
ipad Air or later