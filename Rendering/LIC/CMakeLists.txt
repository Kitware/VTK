set(Module_SRCS
  vtkImageDataLIC2D.cxx
  vtkLineIntegralConvolution2D.cxx
  vtkPixelTransfer.cxx
  vtkStructuredGridLIC2D.cxx
  vtkSurfaceLICComposite.cxx
  vtkSurfaceLICDefaultPainter.cxx
  vtkSurfaceLICPainter.cxx
  vtkTextureIO.cxx
  )

set_source_files_properties(
  vtkLineIntegralConvolution2D.cxx
  vtkPainterCommunicator.cxx
  vtkPixelTransfer.cxx
  vtkSurfaceLICComposite.cxx
  vtkTextureIO.cxx
  WRAP_EXCLUDE
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
  vtkSurfaceLICPainter_GeomVs.glsl
  vtkSurfaceLICPainter_GeomFs.glsl
  vtkSurfaceLICPainter_SC.glsl
  vtkSurfaceLICPainter_CE.glsl
  vtkSurfaceLICPainter_DCpy.glsl
  )

unset(shader_h_files)
foreach(file ${shader_files})
  get_filename_component(file_we ${file} NAME_WE)
  set(src ${CMAKE_CURRENT_SOURCE_DIR}/${file})
  set(res ${CMAKE_CURRENT_BINARY_DIR}/${file_we}.cxx)
  set(resh ${CMAKE_CURRENT_BINARY_DIR}/${file_we}.h)
  list(APPEND shader_h_files ${resh})
  add_custom_command(
    OUTPUT ${res} ${resh}
    DEPENDS ${src} vtkEncodeString
    COMMAND vtkEncodeString
    ARGS ${res} ${src} ${file_we}
    --build-header VTKRENDERINGLIC_EXPORT vtkRenderingLICModule.h
    )
  list(APPEND Module_SRCS ${res})
  set_source_files_properties(${file_we} WRAP_EXCLUDE)
endforeach()

vtk_module_library(${vtk-module} ${Module_SRCS})

include_directories(${OPENGL_INCLUDE_DIR})
