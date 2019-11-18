/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastUserShader2.cxx

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

#include <TestGPURayCastUserShader2_FS.h>

int TestGPURayCastUserShader2(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // Load data
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/tooth.nhdr");
  vtkNew<vtkNrrdReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  vtkImageData* im = reader->GetOutput();
  double* bounds = im->GetBounds();
  double depthRange[2];
  depthRange[0] = vtkMath::Min(bounds[0], bounds[2]);
  depthRange[0] = vtkMath::Min(depthRange[0], bounds[4]);
  depthRange[1] = vtkMath::Max(bounds[1], bounds[3]);
  depthRange[1] = vtkMath::Max(depthRange[1], bounds[5]);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);

  vtkDataArray* arr = reader->GetOutput()->GetPointData()->GetScalars();
  double range[2];
  arr->GetRange(range);

  // Prepare 1D Transfer Functions
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(depthRange[0], 1.0, 0.0, 0.0);
  ctf->AddRGBPoint(0.5 * (depthRange[0] + depthRange[1]), 0.5, 0.5, 0.5);
  ctf->AddRGBPoint(0.8 * (depthRange[0] + depthRange[1]), 0.5, 0.4, 0.6);
  ctf->AddRGBPoint(depthRange[1], 0.0, 1.0, 1.0);

  vtkNew<vtkPiecewiseFunction> pf;
  pf->AddPoint(0, 0.00);
  pf->AddPoint(510, 0.00);
  pf->AddPoint(640, 0.5);
  pf->AddPoint(range[1], 0.5);

  volumeProperty->SetScalarOpacity(pf.GetPointer());
  volumeProperty->SetColor(ctf.GetPointer());

  vtkNew<vtkOpenGLGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SetUseJittering(0);

  // Tell the mapper to use the min and max of the color function nodes as the
  // lookup table range instead of the volume scalar range.
  mapper->SetColorRangeType(vtkGPUVolumeRayCastMapper::NATIVE);

  vtkNew<vtkShaderProperty> shaderProperty;
  // Clear all custom shader tag replacements
  // The following code is mainly for regression testing as we do not have any
  // custom shader replacements.
  shaderProperty->ClearAllVertexShaderReplacements();
  shaderProperty->ClearAllFragmentShaderReplacements();
  shaderProperty->ClearAllGeometryShaderReplacements();
  shaderProperty->ClearAllShaderReplacements();

  // Modify the shader to color based on the depth of the translucent voxel
  shaderProperty->SetFragmentShaderCode(TestGPURayCastUserShader2_FS);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());
  volume->SetShaderProperty(shaderProperty);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(300, 300); // Intentional NPOT size

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  ren->AddVolume(volume.GetPointer());
  ren->GetActiveCamera()->Elevation(-50.0);
  ren->GetActiveCamera()->Yaw(-30.0);
  ren->GetActiveCamera()->Roll(-10.0);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.4);

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
