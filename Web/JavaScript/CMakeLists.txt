vtk_module_impl()
vtk_module_export("")

find_package(PythonInterp REQUIRED)

if(NOT vtkWeb_WWW_DEST)
  set(vtkWeb_WWW_DEST "${VTK_WWW_DIR}")
endif()

set(lib_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/Lib")
set(lib_binary_dir "${vtkWeb_WWW_DEST}/lib")
set(ext_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/Ext")
set(ext_binary_dir "${vtkWeb_WWW_DEST}/ext")
set(lib_js_min_files
  "${lib_source_dir}/vtkweb.js"
  "${lib_source_dir}/vtkweb.launcher.js"
  "${lib_source_dir}/vtkweb.connect.js"
  "${lib_source_dir}/vtkweb.viewport.js"
  "${lib_source_dir}/vtkweb.viewport.image.js"
  "${lib_source_dir}/vtkweb.viewport.vgl.js"
  "${lib_source_dir}/vtkweb.viewport.webgl.js"
  "${lib_source_dir}/vtkweb.simple.js"
  )
if(BUILD_TESTING)
  set(lib_js_min_files
    ${lib_js_min_files}
    "${lib_source_dir}/vtkweb.testing.js"
    )
endif()
set(vtkweb_all_min_js "${lib_binary_dir}/core/vtkweb-all.min.js")
set(vtkweb_all_js "${lib_binary_dir}/core/vtkweb-all.js")
set(vtkweb_loader_js "${lib_binary_dir}/core/vtkweb-loader.js")
set(vtkweb_loader_min_js "${lib_binary_dir}/core/vtkweb-loader-min.js")

set(args)
list(APPEND args "-b" "${CMAKE_CURRENT_SOURCE_DIR}/banner.js.in")
list(APPEND args "-v" "v2.0")
list(APPEND args "-i")

set(min_all_args ${args})
foreach(_file ${lib_js_min_files})
  list(APPEND min_all_args "${_file}")
endforeach()

list(APPEND min_all_args "-o" ${vtkweb_all_min_js})
list(APPEND min_all_args "-m" ${vtkweb_all_js})

add_custom_command(OUTPUT ${vtkweb_all_min_js} ${vtkweb_all_js}
  COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/Minimizer/build.py" ${min_all_args}
  DEPENDS ${lib_js_min_files})

add_custom_command(OUTPUT ${vtkweb_loader_js} ${vtkweb_loader_min_js}
  COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/Minimizer/build.py" ${args}
  "${lib_source_dir}/vtkweb-loader.js" -o ${vtkweb_loader_min_js} -m ${vtkweb_loader_js}
  DEPENDS "${lib_source_dir}/vtkweb-loader.js")

# Copy Ext js files
include(vtkPythonPackages)
copy_files_recursive(${ext_source_dir}
  DESTINATION ${ext_binary_dir}
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ext-js-copy-complete"
  LABEL "Copying JavaScript files"
)

copy_files_recursive("${CMAKE_CURRENT_SOURCE_DIR}/Widgets"
  DESTINATION ${lib_binary_dir}/widgets
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/widget-copy-complete"
  LABEL "Copying Widgets files"
)

add_custom_target(vtkWebJavaScript ALL DEPENDS
  ${vtkweb_all_min_js} ${vtkweb_loader_min_js}
  "${CMAKE_CURRENT_BINARY_DIR}/ext-js-copy-complete"
  "${CMAKE_CURRENT_BINARY_DIR}/widget-copy-complete"
)
