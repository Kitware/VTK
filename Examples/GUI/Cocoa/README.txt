SimpleCocoaVTK 1.0
2005-12-15
by: Sean McBride and Mike Jackson

Pre-Requisites:

1) You need to know the very basics of vtk & Cocoa already.
2) you need Xcode 2.2 installed on your Mac.
3) you have to already have vtk 5 built.  This is a whole process in itself, and is not described here. This project expects that you built vtk as static libraries.

Purpose of Example:

This simple example is intended to show how to use vtk in a Mac-only Cocoa application.  The class vtkCocoaGLView is a subclass of NSView and you can use it like pretty much any other NSView!

This example is an NSDocument-based application, and has two independent vtkCocoaGLViews in the document's NIB.  One shows a cone, and one shows a cylinder.

See the screenshot adjacent to this read me to see what the app looks like.

Instructions:

First, you need to create two Source Trees in Xcode, one named "vtk-include" and one named "vtk-lib".  Do this from Preferences > Source Trees.  Point "vtk-include" to the folder on your disk that contains the vtk public headers, point "vtk-lib" to the folder that contains the built static libraries.  All the vtk files in the project are referenced relative to one of the source trees.  This allows you to have your vtk files anywhere without changing the project itself.

Once that's done, you should be able to build the project.

The code has many comments, and hopefully they will answer any questions you may have.

If you have any problems, please post to the vtk mailing list.
