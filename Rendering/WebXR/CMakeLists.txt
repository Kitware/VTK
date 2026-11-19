set(classes
  vtkWebXRCamera
  vtkWebXRInteractorStyle
  vtkWebXRModel
  vtkWebXRRenderer
  vtkWebXRRenderWindow
  vtkWebXRRenderWindowInteractor
)

# Included by `vtkWebXRRenderWindowInteractor.h`.
set(headers vtkWebXR.h)

if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  # Generate a dummy .cpp file for the webxr library whenever vtkWebXR.js changes
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vtkWebXR.cpp
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/vtkWebXR.cpp
    DEPENDS vtkWebXR.js)

  set(sources vtkWebXR.cpp)
  set(private_headers vtkWebXRUtilities.h)
  set(webxr_input_files
    vtk_webxr_actions.json
  )
endif ()

vtk_module_add_module(VTK::RenderingWebXR
  CLASSES ${classes}
  HEADERS ${headers}
  PRIVATE_HEADERS ${private_headers}
  SOURCES ${sources}
)

if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  string(CONCAT webxr_library_path
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_LIBDIR}>")
  string(CONCAT webxr_action_files_path
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_DATAROOTDIR}/xr_actions>")

  vtk_module_link_options(VTK::RenderingWebXR PUBLIC
    "SHELL:--js-library ${webxr_library_path}/vtkWebXR.js"
    "SHELL:--embed-file ${webxr_action_files_path}/vtk_webxr_actions.json@vtk_webxr_actions.json")

  install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/vtkWebXR.js
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

  install(
    FILES ${webxr_input_files}
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/xr_actions"
    COMPONENT   runtime
  )
endif ()
