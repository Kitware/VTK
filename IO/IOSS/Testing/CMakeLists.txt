set(datasets
  Data/CGNS/fluid.cgns.4.0
  Data/CGNS/fluid.cgns.4.1
  Data/CGNS/fluid.cgns.4.2
  Data/CGNS/fluid.cgns.4.3
  Data/CGNS/r24_node_tailor_test_output/sparc-volume.cgns.36.00
  Data/Exodus/Assembly-Example.g
  Data/Exodus/biplane_rms_pressure_bs.exo
  Data/Exodus/block_with_attributes.g
  Data/Exodus/can.e.4/can.e.4.0
  Data/Exodus/can.e.4/can.e.4.1
  Data/Exodus/can.e.4/can.e.4.2
  Data/Exodus/can.e.4/can.e.4.3
  Data/Exodus/exo_cell_vec2.e
  Data/Exodus/ExRestarts/blow.e
  Data/Exodus/ExRestarts/blow.e-s.001
  Data/Exodus/ExRestarts/blow.ex-timeseries
  Data/Exodus/ghost.ex2
  Data/Exodus/hello_world_fix-d_frf.frq
  Data/Exodus/RubiksCubeWithRotations_gold.g
  Data/Exodus/SAND2020-4077_O-tri6sWFace2.exo
  Data/Exodus/test-nfaced.exo
  Data/FSIobject/FSIobject.inp.EXODUS_DATA.e-s.00001.4.0
  Data/FSIobject/FSIobject.inp.EXODUS_DATA.e-s.00001.4.1
  Data/FSIobject/FSIobject.inp.EXODUS_DATA.e-s.00001.4.2
  Data/FSIobject/FSIobject.inp.EXODUS_DATA.e-s.00001.4.3
  Data/wedge21.g
)
if (VTK_WRAP_PYTHON)
  list(APPEND datasets
    Data/can.exdg
    Data/disk_out_ref.exdg
  )
endif ()

# function to add conduit data. it passes the dataset_name, num_mpi_nodes, and num_timesteps as arguments
function(vtk_add_conduit_data dataset_name num_mpi_nodes num_timesteps)
  set(local_datasets)
  math(EXPR max_mpi_node "${num_mpi_nodes} - 1")
  foreach (mpi_node RANGE 0 ${max_mpi_node})
    list(APPEND local_datasets
      "Data/${dataset_name}_MPI_${num_mpi_nodes}/initialize_params.conduit_bin.${num_mpi_nodes}.${mpi_node}"
      "Data/${dataset_name}_MPI_${num_mpi_nodes}/initialize_params.conduit_bin.${num_mpi_nodes}.${mpi_node}_json"
      "Data/${dataset_name}_MPI_${num_mpi_nodes}/finalize_params.conduit_bin.${num_mpi_nodes}.${mpi_node}"
      "Data/${dataset_name}_MPI_${num_mpi_nodes}/finalize_params.conduit_bin.${num_mpi_nodes}.${mpi_node}_json"
    )
    math(EXPR max_timestep "${num_timesteps} - 1")
    foreach (timestep RANGE 0 ${max_timestep})
      list(APPEND local_datasets
        "Data/${dataset_name}_MPI_${num_mpi_nodes}/execute_invc${timestep}_params.conduit_bin.${num_mpi_nodes}.${mpi_node}"
        "Data/${dataset_name}_MPI_${num_mpi_nodes}/execute_invc${timestep}_params.conduit_bin.${num_mpi_nodes}.${mpi_node}_json"
      )
    endforeach ()
  endforeach ()
  set(datasets "${datasets};${local_datasets}" PARENT_SCOPE)
endfunction()

vtk_add_conduit_data(Iocatalyst_can_ex2 1 44)
vtk_add_conduit_data(Iocatalyst_can_ex2 4 44)
vtk_add_conduit_data(Iocatalyst_sparc1_cgns 1 3)

if (VTK_USE_LARGE_DATA)
  list(APPEND datasets
    Data/Exodus/SAND2017-5827O-FSM_Residual-bad-eigen.e
    Data/Exodus/SAND2017-5827O-FSM_Residual_good-eigen.e)
endif()
vtk_module_test_data(${datasets})

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()
if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
