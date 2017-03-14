Welcome to the android examples for VTK. Currently the android support is at
a very early stage but hopefully these examples can give you a head start on
developing your own android applications.

Currently there are three examples here.  NativeVTK is built around the
Android native interface and does not have any Java code associated with it.
Any unser interface elements would need to be created in C++ using regular
VTK widgets or other OpenGL elements. This is best for applications that
are mainly focused on rendering or visualization with very minimal user
interface. In this example all the key code is in jni/main.cxx and it will
look very much like the regular VTK code you are used to.

The second example is JavaVTK.  In this example the OpenGL context is
created and managed in the Java layer as an android view/surface. You can
add other java based user interface components as desired. Rendering
is handled by VTK.  In the JavaVTK example we forward keyboard and
motion events to a VTK interactor for processing but you could choose
to handle them yourself and maniputlate the camera directly.

In this example the code is a bit more distributed. The C++ code is in
jni/main.cxx while the Java/C++ interface API is defined in JavaVTKLib.java
The bulk of the actual application logic is in JavaVTKView.java

The third example is VolumeRender and this example requires OpenGL ES 3.0
support.  Most newer android devices should support OpenGL ES 3.0.

To build VTK and these examples follow the steps below.

* do this on Linux or OSX. The process on Windows is complicated

* make sure you have the android NDK installed, set the ANDROID_NDK environment
* variable to where you installed the NDK

* make sure you have a recent android SDK installed, add the sdk/platform-tools
* and sdk/tools directories to your path

* install ant, it is required as part of the example build process

* Create a binary directory for VTK. Typically I will create vtkandroid next to my vtk source tree.

* Run cmake on vtkandroid with -DVTK_ANDROID_BUILD=ON, if you use the gui add a
* boolean entry with that name prior to configuring and set it on.

If you want OpenGL ES 3.0 support make sure to change the setting of
OPENGL_ES_VERSION to 3.0. Volume Rendering requires ES 3.0. Make sure to turn on
BUILD_EXAMPLES

* configure and generate as usual

* Once done run ninja or make as appropriate

That should build VTK and the android examples. To try running an example you
should put your android device into developer mode (how to do this varies based
on device but usually it involves finding the android build number somewhere in
the settings on your device. Then tap that number seven times.

Connect your device to your computer via USB.

cd into CMakeExternals/Build/vtk-android/Examples/Android/ExampleName/bin

You should see some apk files in this directory.

You can adb install -r ExampleName-debug.apk and then run the example on your device