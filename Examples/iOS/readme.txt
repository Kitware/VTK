*** Danger Danger Danger ***

The iOS build of VTK is in an alpha state. Please do try it out but understand
it is very new.

To build a VTK application for iOS you will need a Mac/Macbook that has Xcode on
it and the iOS dev kit. What follows are the basic steps:


* Make a directory for the host binaries (e.g. vtkbin). I typically create this
* next to the VTK source tree.

* Build VTK the normal way into that directory, no cross compiling, nothing
* fancy, you just need a build of VTK on the host system for a few executables
* VTK uses as part of its build process.

* Make a directory for the iOS binaries to go into (your binary tree). I
* usually create a directory next to VTK caled vtkiphone, for example.

* Cross compile VTK into this new directory using CMake. Select the Unix
* Makefiles (or Ninja) generator. Select a toolchain when prompted to (or
* specify it on the CMake command line if you are not using the GUI). This is a
* second build of VTK, this time cross compiled. The toolchain files are kept in
* VTK/CMake and are named

    ios.device.toolchain.cmake -- for an iPad or iPhone
    ios.simulator.toolchain.cmake -- for the simulator that runs on the mac

    So, a full command line example is:

    $ cd vtk-ios-device-bin
    $ ccmake
      -DCMAKE_TOOLCHAIN_FILE=../VTK/CMake/ios.device.toolchain.cmake
      -DVTKCompileTools_DIR=../vtk-bin -DBUILD_EXAMPLES=ON -DBUILD_TESTING=OFF
      ../VTK

* Make sure the CMAKE_INSTALL_PREFIX is set to something you are OK with
* installing to. Part of this build process requires you do a make install
* so make sure it is a good place.

* Similarly, make sure that you have write access to the directory specified
* by CMAKE_FRAMEWORK_INSTALL_PREFIX. By default, this directory is
* /usr/local/frameworks, to which you should have write access. The iOS
* examples expect vtk.framework in this directory, so if you change this
* directory you will have to repair the framework reference in the Xcode
* project.

* As part of the Cmake configure process it will ask you to set
* VTKCompileTools_DIR, set this to the binary tree from your first build, e.g.
* vtkbin

* Build by running make or ninja.

* Run 'make install' (or 'ninja install') to install your just-built VTK.

* Run 'make framework' (or 'ninja framework') to create vtk.framework from your
* current VTK installation.

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

The app is very simple with most of the opengl setup etc being handled by the
vtkIOSView class in VTK. The pipeline is setup in the class PaintingView.mm in
the usual VTK way. The superclass will automatically create a render window and
interactor for you.

The GLPaint example is a modified version of Apple's GLPaint example. So credit
to them for the original example file and framework.
