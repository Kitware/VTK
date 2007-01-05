SimpleCocoaVTK 1.1
2007-01-04
by: Sean McBride and Mike Jackson
This project is public domain.

Pre-Requisites:

1) you need Xcode 2.2 or later installed on your Mac.
2) you need to know the very basics of vtk & Cocoa already.
3) you have to build vtk seperately.  This project expects vtk to be built as static libraries.  If you've never built vtk before, please see Ryan Glover's excellent document here: <http://www.vtk.org/Wiki/Cocoa_VTK>.

Purpose of Example:

This simple example is intended to show how to use vtk in a Mac-only Cocoa application.  The class vtkCocoaGLView is a subclass of NSView and you can use it like pretty much any other NSView!

This example is an NSDocument-based application, and has two independent vtkCocoaGLViews in the document's NIB.  One shows a cone, and one shows a cylinder.

See the screenshot adjacent to this read me to see what the app looks like.

Instructions:

First, you need to create two Source Trees in Xcode, one named "vtk-include" and one named "vtk-lib".  Do this from Preferences > Source Trees.  Point "vtk-include" to the folder on your disk that contains the vtk public headers, point "vtk-lib" to the folder that contains the built static libraries. It is expected that you have built vtk as static libraries and that you have 'installed' vtk such that you have one folder with all the static libraries and one folder that contains all the headers (flat, not as a hierarchy). This is the layout vtk creates when you 'make install'.

All the vtk files in the project are referenced relative to one of the source trees.  This allows you to have your vtk files anywhere without changing the project itself.

Once that's done, you should be able to build the project.

The code has many comments, and hopefully they will answer any questions you may have.

If you have any problems, please post to the vtk mailing list.
<http://public.kitware.com/mailman/listinfo/vtkusers>

Version History:

Changes in 1.1 (since 1.0)
- fixed some memory leaks
- minor code cleanup
- added call to print out any leaking vtk objects
- linked to Ryan Glover's great instructions on how to build vtk
