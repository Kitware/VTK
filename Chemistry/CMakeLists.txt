SET(KIT Chemistry)
SET(UKIT CHEMISTRY)

SET(KIT_LIBS vtkRendering vtkFiltering)

SET(Kit_SRCS
  vtkBlueObeliskData.cxx
  vtkBlueObeliskDataParser.cxx
  vtkCMLMoleculeReader.cxx
  vtkPeriodicTable.cxx
  )

#-----------------------------------------------------------------------------
# Include CMake code common to all kits.
INCLUDE(${VTK_CMAKE_DIR}/KitCommonBlock.cmake)
#-----------------------------------------------------------------------------

# Install Blue Obelisk Data Repository files
INSTALL(DIRECTORY vtkBODRData
  DESTINATION ${VTK_INSTALL_SHARE_DIR_CM24}/vtkChemistry
  )

# Generate configured header file
CONFIGURE_FILE(${VTK_SOURCE_DIR}/Chemistry/vtkChemistryConfigure.h.in
               ${VTK_BINARY_DIR}/Chemistry/vtkChemistryConfigure.h IMMEDIATE)
