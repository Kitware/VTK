Welcome to the android examples for VTK. Currently the android support is at
a very early stage but hopefully these examples can give you a head start on
developing your own android applications.

Currently there are two examples here.  NativeVTK is built around the
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
The bulk of the actual applicaiton logic is in JavaVTKView.java