vtk_module_test_data(
  Data/SemiDisk/SemiDisk-0.vtp
  Data/SemiDisk/SemiDisk-1.vtp)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/SainteHelens.dem
    Data/headsq/,REGEX:.*
    Data/ironProt.vtk
    Data/nut.slc
    Data/ruler.png
    Data/usa.vtk
    Data/vtk.vtk)

  add_subdirectory(Python)
endif ()
