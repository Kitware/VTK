#
# Add the executable
#

ADD_EXECUTABLE(vtkmyEx1 vtkmyEx1)

#
# Link to VTK and local
#

LINK_LIBRARIES (
  vtkmyUnsorted
  vtkHybrid 
  vtkRendering
  vtkIO
  vtkGraphics
  vtkmyImaging 
  vtkImaging 
  vtkFiltering
  vtkmyCommon
  vtkCommon
)
