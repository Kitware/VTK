/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastUserShader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// Test the volume mapper's ability to perform shader substitutions based on
// user specified strings.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkNrrdReader.h"
#include "vtkOpenGLGPUVolumeRayCastMapper.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkShaderProperty.h"
#include "vtkTestUtilities.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

int TestGPURayCastUserShader(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // Load data
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/tooth.nhdr");
  vtkNew<vtkNrrdReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);

  vtkDataArray* arr = reader->GetOutput()->GetPointData()->GetScalars();
  double range[2];
  arr->GetRange(range);

  // Prepare 1D Transfer Functions
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0, 0.0, 0.0, 0.0);
  ctf->AddRGBPoint(510, 0.4, 0.4, 1.0);
  ctf->AddRGBPoint(640, 1.0, 1.0, 1.0);
  ctf->AddRGBPoint(range[1], 0.9, 0.1, 0.1);

  vtkNew<vtkPiecewiseFunction> pf;
  pf->AddPoint(0, 0.00);
  pf->AddPoint(510, 0.00);
  pf->AddPoint(640, 0.5);
  pf->AddPoint(range[1], 0.4);

  volumeProperty->SetScalarOpacity(pf.GetPointer());
  volumeProperty->SetColor(ctf.GetPointer());
  volumeProperty->SetShade(1);

  vtkNew<vtkOpenGLGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SetUseJittering(1);

  vtkNew<vtkShaderProperty> shaderProperty;

  // Modify the shader to color based on the depth of the translucent voxel
  shaderProperty->AddFragmentShaderReplacement("//VTK::Base::Dec", // Source string to replace
    true,              // before the standard replacements
    "//VTK::Base::Dec" // We still want the default
    "\n bool l_updateDepth;"
    "\n vec3 l_opaqueFragPos;",
    false // only do it once i.e. only replace the first match
  );
  shaderProperty->AddFragmentShaderReplacement("//VTK::Base::Init", true,
    "//VTK::Base::Init\n"
    "\n l_updateDepth = true;"
    "\n l_opaqueFragPos = vec3(0.0);",
    false);
  shaderProperty->AddFragmentShaderReplacement("//VTK::Base::Impl", true,
    "//VTK::Base::Impl"
    "\n    if(!g_skip && g_srcColor.a > 0.0 && l_updateDepth)"
    "\n      {"
    "\n      l_opaqueFragPos = g_dataPos;"
    "\n      l_updateDepth = false;"
    "\n      }",
    false);
  shaderProperty->AddFragmentShaderReplacement("//VTK::RenderToImage::Exit", true,
    "//VTK::RenderToImage::Exit"
    "\n  if (l_opaqueFragPos == vec3(0.0))"
    "\n    {"
    "\n    fragOutput0 = vec4(0.0);"
    "\n    }"
    "\n  else"
    "\n    {"
    "\n    vec4 depthValue = in_projectionMatrix * in_modelViewMatrix *"
    "\n                      in_volumeMatrix[0] * in_textureDatasetMatrix[0] *"
    "\n                      vec4(l_opaqueFragPos, 1.0);"
    "\n    depthValue /= depthValue.w;"
    "\n    fragOutput0 = vec4(vec3(0.5 * (gl_DepthRange.far -"
    "\n                       gl_DepthRange.near) * depthValue.z + 0.5 *"
    "\n                      (gl_DepthRange.far + gl_DepthRange.near)), 1.0);"
    "\n    }",
    false);
  shaderProperty->AddFragmentShaderReplacement( // add dummy replacement
    "//VTK::ComputeGradient::Dec", true, "VTK::ComputeGradient::Dec", false);
  shaderProperty->ClearFragmentShaderReplacement( // clear dummy replacement
    "//VTK::ComputeGradient::Dec", true);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());
  volume->SetShaderProperty(shaderProperty.GetPointer());

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(300, 300); // Intentional NPOT size

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  ren->AddVolume(volume.GetPointer());
  ren->GetActiveCamera()->Elevation(-60.0);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.3);

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
