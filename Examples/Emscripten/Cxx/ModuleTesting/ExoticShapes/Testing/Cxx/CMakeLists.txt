set(_vtk_test_cxx_wasm_enabled_in_browser ON)

vtk_add_test_cxx(BarCustomModuleCxxTests tests TestHelix.cxx,NO_DATA,NO_VALID)

vtk_test_cxx_executable(BarCustomModuleCxxTests tests
    RENDERING_FACTORY
)
