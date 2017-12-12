set(Module_SRCS
  vtkXdmf3ArrayKeeper.cxx
  vtkXdmf3ArraySelection.cxx
  vtkXdmf3DataSet.cxx
  vtkXdmf3HeavyDataHandler.cxx
  vtkXdmf3LightDataHandler.cxx
  vtkXdmf3Reader.cxx
  vtkXdmf3SILBuilder.cxx
  vtkXdmf3Writer.cxx
  )

include_directories(${XdmfCore_INCLUDE_DIRS} ${XdmfDSM_INCLUDE_DIRS})

mark_as_advanced(CLEAR Boost_INCLUDE_DIR)
find_package(Boost REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  mark_as_advanced(FORCE Boost_INCLUDE_DIR)
endif(Boost_FOUND)

vtk_module_library(vtkIOXdmf3 ${Module_SRCS})
