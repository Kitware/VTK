vtk_module_impl()
vtk_module_export("")

find_package(PythonInterp ${VTK_PYTHON_VERSION} REQUIRED)

if(NOT vtkWeb_WWW_DEST)
  set(vtkWeb_WWW_DEST "${VTK_WWW_DIR}")
endif()

set(lib_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/Lib")
set(lib_binary_dir "${vtkWeb_WWW_DEST}/lib")

set(ext_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/Ext")
set(ext_binary_dir "${vtkWeb_WWW_DEST}/ext")

set(widgets_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/Widgets")
set(widgets_binary_dir "${vtkWeb_WWW_DEST}/lib/widgets")

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

set(widgets_js_min_files
  "${widgets_source_dir}/CatalystBrowser/vtkweb-widget-catalyst-analysis-cost-estimate.js"
  "${widgets_source_dir}/CatalystBrowser/vtkweb-widget-catalyst-analysis-cost.js"
  "${widgets_source_dir}/CatalystBrowser/vtkweb-widget-catalyst-analysis-bench.js"
  "${widgets_source_dir}/CatalystBrowser/vtkweb-widget-catalyst-viewer.js"
  "${widgets_source_dir}/CatalystBrowser/vtkweb-widget-catalyst-resampler-viewer.js"
  "${widgets_source_dir}/CatalystBrowser/vtkweb-widget-catalyst-composite.js"
  "${widgets_source_dir}/CatalystBrowser/vtkweb-widget-catalyst-pvweb.js"
  "${widgets_source_dir}/CatalystBrowser/vtkweb-widget-catalyst-analysis-search.js"
  "${widgets_source_dir}/CatalystBrowser/vtkweb-widget-catalyst-analysis.js"
  "${widgets_source_dir}/ChartWidget/vtkweb-widget-chart.js"
  "${widgets_source_dir}/FileBrowser/vtkweb-widget-filebrowser.js"
  "${widgets_source_dir}/TreeWidget/vtkweb-widget-tree.js"
)
set(widgets_css_min_files
  "${widgets_source_dir}/CatalystBrowser/vtkweb-widget-catalyst-analysis-cost-estimate.css"
  "${widgets_source_dir}/CatalystBrowser/vtkweb-widget-catalyst-analysis-cost.css"
  "${widgets_source_dir}/CatalystBrowser/vtkweb-widget-catalyst-analysis-bench.css"
  "${widgets_source_dir}/CatalystBrowser/vtkweb-widget-catalyst-viewer.css"
  "${widgets_source_dir}/CatalystBrowser/vtkweb-widget-catalyst-resampler-viewer.css"
  "${widgets_source_dir}/CatalystBrowser/vtkweb-widget-catalyst-composite.css"
  "${widgets_source_dir}/CatalystBrowser/vtkweb-widget-catalyst-pvweb.css"
  "${widgets_source_dir}/CatalystBrowser/vtkweb-widget-catalyst-analysis-search.css"
  "${widgets_source_dir}/CatalystBrowser/vtkweb-widget-catalyst-analysis.css"
  "${widgets_source_dir}/ChartWidget/vtkweb-widget-chart.css"
  "${widgets_source_dir}/FileBrowser/vtkweb-widget-filebrowser.css"
  "${widgets_source_dir}/TreeWidget/vtkweb-widget-tree.css"
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

set(vtkweb_widgets_js "${widgets_binary_dir}/vtkweb-widgets.js")
set(vtkweb_widgets_min_js "${widgets_binary_dir}/vtkweb-widgets-min.js")

set(vtkweb_widgets_css "${widgets_binary_dir}/vtkweb-widgets.css")
set(vtkweb_widgets_min_css "${widgets_binary_dir}/vtkweb-widgets-min.css")

# ====================================================
# === Helper to combine and Pack JavaScripts files ===
# ====================================================
macro(js_pack file_list combine_file_name compress_file_name)
  set(args)
  list(APPEND args "-b" "${CMAKE_CURRENT_SOURCE_DIR}/banner.js.in")
  list(APPEND args "-v" "v2.0")
  list(APPEND args "-i")

  set(min_all_args ${args})
  foreach(_file ${file_list})
    list(APPEND min_all_args "${_file}")
  endforeach()

  list(APPEND min_all_args "-o" ${compress_file_name})
  list(APPEND min_all_args "-m" ${combine_file_name})

  add_custom_command(OUTPUT ${compress_file_name} ${combine_file_name}
    COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/Minimizer/build.py" ${min_all_args}
    DEPENDS ${file_list})

endmacro()
# ====================================================

# Create vtkweb-all
js_pack( "${lib_js_min_files}" "${vtkweb_all_js}" "${vtkweb_all_min_js}")

# Create vtkweb-loader
js_pack( "${lib_source_dir}/vtkweb-loader.js" "${vtkweb_loader_js}" "${vtkweb_loader_min_js}")

# Create widgets
js_pack( "${widgets_js_min_files}" "${vtkweb_widgets_js}" "${vtkweb_widgets_min_js}")
js_pack( "${widgets_css_min_files}" "${vtkweb_widgets_css}" "${vtkweb_widgets_min_css}")

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
  ${vtkweb_all_min_js} ${vtkweb_loader_min_js} ${vtkweb_widgets_min_js} ${vtkweb_widgets_min_css}
  "${CMAKE_CURRENT_BINARY_DIR}/ext-js-copy-complete"
  "${CMAKE_CURRENT_BINARY_DIR}/widget-copy-complete"
)
