PROJECT(WRAPPING)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_SOURCE_DIR}/Wrapping/LocalUserOptions.cmake OPTIONAL)

ADD_EXECUTABLE(vtkWrapTclTest vtkWrapTcl vtkParse.tab)
