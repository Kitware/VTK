## Set the Kit name
SET(KIT AMR)
SET(UKIT AMR)

## Get the kit sources
FILE( GLOB Kit_SRCS *.cxx )

SET_SOURCE_FILES_PROPERTIES(
  vtkAMRGridIndexEncoder
  vtkAMRLink
  WRAP_EXCLUDE
)
## Include CMAKE code common to all kits
INCLUDE(${VTK_CMAKE_DIR}/KitCommonBlock.cmake )