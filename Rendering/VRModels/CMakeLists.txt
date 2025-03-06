set(classes
  vtkOpenGLAvatar
  vtkVRRay)

vtk_object_factory_declare(
  BASE "vtkAvatar"
  OVERRIDE "vtkOpenGLAvatar")

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKRENDERINGVRMODELS_EXPORT")

set(geometry_sources)
set(geometry_headers)

# Generates headers with char arrays that contain the model file contents.
set(geometry_files
  data/vtkAvatarHead.vtp
  data/vtkAvatarLeftForeArm.vtp
  data/vtkAvatarLeftHand.vtp
  data/vtkAvatarLeftUpperArm.vtp
  data/vtkAvatarRightForeArm.vtp
  data/vtkAvatarRightHand.vtp
  data/vtkAvatarRightUpperArm.vtp
  data/vtkAvatarTorso.vtp)
foreach (geometry_file IN LISTS geometry_files)
  vtk_encode_string(
    INPUT         "${geometry_file}"
    EXPORT_HEADER "vtkRenderingVRModelsModule.h"
    EXPORT_SYMBOL "VTKRENDERINGVRMODELS_NO_EXPORT"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source
    BINARY
    NUL_TERMINATE)
  list(APPEND geometry_sources "${source}")
  list(APPEND geometry_headers "${header}")
endforeach ()

vtk_module_add_module(VTK::RenderingVRModels
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source} ${geometry_sources}
  PRIVATE_HEADERS ${vtk_object_factory_header} ${geometry_headers})
