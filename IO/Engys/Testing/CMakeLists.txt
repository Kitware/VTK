if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

vtk_module_test_data(
        Data/Engys/bts/2400-IDGH.bts
        Data/Engys/vtpd/2400-IDGH.vtpd
        Data/Engys/vtpd/2400-IDGH/2400-IDGH_0.vtp
        Data/Engys/bts/multiple_patches.bts
        Data/Engys/vtpd/multiple_patches.vtpd
        Data/Engys/vtpd/multiple_patches/multiple_patches_0.vtp
        Data/Engys/vtpd/multiple_patches/multiple_patches_1.vtp
)
