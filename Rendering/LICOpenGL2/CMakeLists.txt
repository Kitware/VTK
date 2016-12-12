set(Module_SRCS
  vtkImageDataLIC2D.cxx
  vtkLICNoiseHelper.cxx
  vtkLineIntegralConvolution2D.cxx
  vtkPainterCommunicator.h
  vtkStructuredGridLIC2D.cxx
  vtkSurfaceLICComposite.cxx
  vtkSurfaceLICHelper.cxx
  vtkSurfaceLICInterface.cxx
  vtkSurfaceLICMapper.cxx
  vtkCompositeSurfaceLICMapper.cxx
  vtkTextureIO.cxx
  )

set_source_files_properties(
  vtkLICNoiseHelper.cxx
  vtkSurfaceLICHelper.cxx
  vtkPainterCommunicator.h
  vtkSurfaceLICComposite.cxx
  vtkTextureIO.cxx
  PROPERTIES
    WRAP_EXCLUDE 1
    WRAP_EXCLUDE_PYTHON 1
  )

if (VTK_RENDERINGPARALLELLIC_SURFACELICPAINTER_TIMER)
  add_definitions("-DvtkSurfaceLICPainterTIME")
endif()
if (VTK_RENDERINGPARALLELLIC_LINEINTEGRALCONVLOLUTION2D_TIMER)
  add_definitions("-DvtkLineIntegralConvolution2DTIME")
endif()

set(shader_files
  vtkLineIntegralConvolution2D_VT.glsl
  vtkLineIntegralConvolution2D_LIC0.glsl
  vtkLineIntegralConvolution2D_LICI.glsl
  vtkLineIntegralConvolution2D_LICN.glsl
  vtkLineIntegralConvolution2D_EE.glsl
  vtkLineIntegralConvolution2D_CE.glsl
  vtkLineIntegralConvolution2D_AAH.glsl
  vtkLineIntegralConvolution2D_AAV.glsl
  vtkStructuredGridLIC2D_fs.glsl
  vtkSurfaceLICInterface_SC.glsl
  vtkSurfaceLICInterface_CE.glsl
  vtkSurfaceLICInterface_DCpy.glsl
  )
unset(shader_h_files)
foreach(file ${shader_files})
  get_filename_component(file_we ${file} NAME_WE)
  set(src  ${CMAKE_CURRENT_SOURCE_DIR}/${file})
  set(res  ${CMAKE_CURRENT_BINARY_DIR}/${file_we}.cxx)
  set(resh ${CMAKE_CURRENT_BINARY_DIR}/${file_we}.h)
  list(APPEND shader_h_files ${resh})
  add_custom_command(
    OUTPUT ${res} ${resh}
    DEPENDS ${src} vtkEncodeString
    COMMAND vtkEncodeString
    ARGS ${res} ${src} ${file_we}
      --build-header VTKRENDERINGLICOPENGL2_EXPORT vtkRenderingLICOpenGL2Module.h
    )
  list(APPEND Module_SRCS ${res})
  set_source_files_properties(${file_we}
    PROPERTIES
      WRAP_EXCLUDE 1
      WRAP_EXCLUDE_PYTHON 1
    )
endforeach()


vtk_module_library(${vtk-module} ${Module_SRCS})

include(vtkOpenGL)
vtk_opengl_link(${vtk-module})
