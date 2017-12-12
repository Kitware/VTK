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
  vtk_encode_string(
    INPUT         "${file}"
    EXPORT_SYMBOL "VTKRENDERINGLICOPENGL2_EXPORT"
    EXPORT_HEADER "vtkRenderingLICOpenGL2Module.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND Module_SRCS ${source})
endforeach()


vtk_module_library(${vtk-module} ${Module_SRCS})

include(vtkOpenGL)
vtk_opengl_link(${vtk-module})
