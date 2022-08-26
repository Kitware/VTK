if (WIN32)
  # Create imported target DirectX::d3d11
  if (NOT TARGET DirectX::d3d11)
    add_library(DirectX::d3d11 INTERFACE IMPORTED)
    set_target_properties(DirectX::d3d11 PROPERTIES IMPORTED_LIBNAME "d3d11.lib")
  endif()

  # Create imported target DirectX::dxgi
  if (NOT TARGET DirectX::dxgi)
    add_library(DirectX::dxgi INTERFACE IMPORTED)
    set_target_properties(DirectX::dxgi PROPERTIES IMPORTED_LIBNAME "dxgi.lib")
  endif()

  set(DirectX_FOUND TRUE)
else()
  set(DirectX_FOUND FALSE)
  set(DirectX_NOT_FOUND_MESSAGE "Unsupported platform")
endif()
