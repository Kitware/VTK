# build zlib, png, jpeg, and tiff, expat, Doxygen doc, freetype, ftgl
SUBDIRS(zlib png jpeg tiff expat Doxygen freetype ftgl)

# This is the installed VTKConfig.cmake file configured at the top
# level.
INSTALL_FILES(/lib/vtk .cmake VTKConfig)

