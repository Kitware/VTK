# this needs lots of work!  Doesn't include NIBs, plist, etc etc etc

PROJECT(SimpleCocoa)

FIND_PACKAGE(VTK REQUIRED)
INCLUDE(${VTK_USE_FILE})

SET(MACOSX_BUNDLE_INFO_STRING "Simple Cocoa VTK")
ADD_EXECUTABLE(SimpleCocoaVTK MACOSX_BUNDLE main.m BasicVTKView.mm MyDocument.mm)
TARGET_LINK_LIBRARIES(SimpleCocoaVTK vtkRendering)

