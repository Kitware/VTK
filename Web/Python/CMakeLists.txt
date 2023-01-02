set(files
  vtkmodules/web/__init__.py
  vtkmodules/web/camera.py
  vtkmodules/web/dataset_builder.py
  vtkmodules/web/errors.py
  vtkmodules/web/protocols.py
  vtkmodules/web/query_data_model.py
  vtkmodules/web/render_window_serializer.py
  vtkmodules/web/testing.py
  vtkmodules/web/vtkjs_helper.py
  vtkmodules/web/venv.py
  vtkmodules/web/wslink.py
  vtkmodules/web/utils.py)

vtk_module_add_python_package(VTK::WebPython
  FILES               ${files}
  PACKAGE             "vtkmodules.web"
  MODULE_DESTINATION  "${VTK_PYTHON_SITE_PACKAGES_SUFFIX}")

vtk_module_add_python_module(VTK::WebPython
  PACKAGES "vtkmodules.web")

set_property(GLOBAL APPEND
  PROPERTY
    vtk_web_python_modules "wslink>=1.0.4")
