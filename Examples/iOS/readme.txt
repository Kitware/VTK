
*** Danger Danger Danger ***

The iOS build of VTK is in an alpha state. Please do try it out but understand it is very new.

To build a VTK application for iOS you will need a Mac/Macbook that has Xcode on it and the iOS dev kit. What follows are the basic steps:

* make a directory for the binaries to go into (your binary tree).  I usually create a directory next to VTK caled vtkiphone for example.

* Build VTK using CMake and select the Unix Makefiles (or Ninja) and select a toolchain when prompted to (or specify it on the cmake command line if you are not using the GUI. The toolchain files are kept in VTK/CMake and are named

  ios.device.toolchain.cmake -- for an ipan or iphone
  ios.simulator.toolchain.cmake -- for the simulator that run on the mac

* Make sure the CMAKE_INSTALL_PREFIX is set to somethign you are OK with installing to. Part of this build process required you do a make install so make sure it is a good place.

* Build VTK

* do a make install (or ninja instal) to install VTK

* DANGER: look at the following shell script before running it to make sure it will not do a rm -r * or anything like that.  It should be fine, but there are configured values and it does delete file, so I get nervous.

* make a framework out of vtk by running the shell script make_vtk_framework.sh which can be found in your binary tree under Examples/iOS

* load the GLPaint xcode project from your binary tree into XCode

* make sure you have any provisiong certificates, devices setiup properly, Apple requires iOS developers to have apple issued certificates and their devices have to be registered before they can be used to test apps. So do all that stuff. Have fun, it is a bit hellish but there are guides out on the internet. If you are already doing iOS app developent and testing then you shoudl be good.

* build it in xcode and then run it

* enjoy a delicious mudslide, you earned it

The app is very simple with most of the opengl setup etc being handled by the vtkIOSView class in VTK. The pipeline is setup in the class PaintingView.mm in the usual VTK way. The superclass will authomatically create a renderwindow and interactor for you.

The GLPaint example is a modified version of Apple's GLPaint example. So credit to them for the original example file and framework.
