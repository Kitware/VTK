# Tests with data
ExternalData_Expand_Arguments(VTKData _
  "DATA{../Data/Input/,RECURSE:,REGEX:.*}"
  )

# We don't use `vtk_add_test_python_mpi` since we need to customize the
# arguments and number of ranks these tests are run on.

ExternalData_add_test(VTKData
  NAME TestParalleComputeShort-p4
  COMMAND ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 4 ${VTK_MPI_PREFLAGS}
          $<TARGET_FILE:pvtkpython>
          ${CMAKE_CURRENT_SOURCE_DIR}/TestParalleComputeShort.py
          DATA{../Data/Input/2DScalar.vti}
          DATA{../Data/Input/2DScalar.vtm}
)

ExternalData_add_test(VTKData
  NAME TestParalleComputeShort-p8
  COMMAND ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 8 ${VTK_MPI_PREFLAGS}
          $<TARGET_FILE:pvtkpython>
          ${CMAKE_CURRENT_SOURCE_DIR}/TestParalleComputeShort.py
          DATA{../Data/Input/3DScalar.vti}
          DATA{../Data/Input/3DScalar.vtm}
)
