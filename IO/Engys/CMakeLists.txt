set(classes
  vtkBTSReader
  )

vtk_module_add_module(VTK::IOEngys
        CLASSES ${classes})

vtk_add_test_mangling(VTK::IOEngys)
