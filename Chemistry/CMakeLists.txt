INCLUDE(vtkDependentOption)

SET(KIT Chemistry)
SET(UKIT CHEMISTRY)

SET(KIT_LIBS vtkRendering vtkFiltering vtkVolumeRendering)

SET(Kit_SRCS
  vtkAbstractElectronicData.cxx
  vtkBlueObeliskData.cxx
  vtkBlueObeliskDataParser.cxx
  vtkCMLMoleculeReader.cxx
  vtkMoleculeAlgorithm.cxx
  vtkMoleculeMapper.cxx
  vtkMoleculeToAtomBallFilter.cxx
  vtkMoleculeToBondStickFilter.cxx
  vtkMoleculeToPolyDataFilter.cxx
  vtkPeriodicTable.cxx
  vtkProgrammableElectronicData.cxx
  vtkSimpleBondPerceiver.cxx
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkAbstractElectronicData.cxx
  vtkMoleculeToPolyDataFilter.cxx
  ABSTRACT
  )

# OpenQube setup
FIND_PACKAGE(OpenQube QUIET)
IF(OpenQube_FOUND)
  FIND_PACKAGE(Eigen2 QUIET)
ENDIF(OpenQube_FOUND)
VTK_DEPENDENT_OPTION(VTK_USE_OPENQUBE
                     "Compile OpenQube support into vtkChemistry."
                     ON "OpenQube_FOUND;EIGEN2_FOUND" OFF)
IF(VTK_USE_OPENQUBE)
  INCLUDE_DIRECTORIES(${OpenQube_INCLUDE_DIRS} ${EIGEN2_INCLUDE_DIR})
  SET(KIT_LIBS ${KIT_LIBS} OpenQube)
  SET(Kit_SRCS
    ${Kit_SRCS}
    vtkOpenQubeElectronicData.cxx
    vtkOpenQubeMoleculeSource.cxx
  )
ENDIF(VTK_USE_OPENQUBE)

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
