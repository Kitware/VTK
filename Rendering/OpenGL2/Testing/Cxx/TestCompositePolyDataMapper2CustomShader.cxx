/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCompositePolyDataMapper2NaNPartial.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCompositePolyDataMapper2.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkFloatArray.h"
#include "vtkLookupTable.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkShaderProperty.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"

#include <vtkRegressionTestImage.h>
#include <vtkTestUtilities.h>

void FillShaderProperty(vtkActor* actor)
{
  // Modify the shader to color based on model normal
  // To do this we have to modify the vertex shader
  // to pass the normal in model coordinates
  // through to the fragment shader. By default the normal
  // is converted to View coordinates and then passed on.
  // We keep that, but add a varying for the original normal.
  // Then we modify the fragment shader to set the diffuse color
  // based on that normal. First lets modify the vertex
  // shader
  vtkShaderProperty* sp = actor->GetShaderProperty();
  sp->AddVertexShaderReplacement("//VTK::Normal::Dec", // replace the normal block
    true,                                              // before the standard replacements
    "//VTK::Normal::Dec\n"                             // we still want the default
    "  out vec3 myNormalMCVSOutput;\n",                // but we add this
    false                                              // only do it once
  );
  sp->AddVertexShaderReplacement("//VTK::Normal::Impl", // replace the normal block
    true,                                               // before the standard replacements
    "//VTK::Normal::Impl\n"                             // we still want the default
    "  myNormalMCVSOutput = normalMC;\n",               // but we add this
    false                                               // only do it once
  );
  sp->AddVertexShaderReplacement("//VTK::Color::Impl", // dummy replacement for testing clear method
    true, "VTK::Color::Impl\n", false);
  sp->ClearVertexShaderReplacement("//VTK::Color::Impl", true);

  // now modify the fragment shader
  sp->AddFragmentShaderReplacement("//VTK::Normal::Dec", // replace the normal block
    true,                                                // before the standard replacements
    "//VTK::Normal::Dec\n"                               // we still want the default
    "  in vec3 myNormalMCVSOutput;\n",                   // but we add this
    false                                                // only do it once
  );
  sp->AddFragmentShaderReplacement("//VTK::Normal::Impl", // replace the normal block
    true,                                                 // before the standard replacements
    "//VTK::Normal::Impl\n"                               // we still want the default calc
    "  diffuseColor = abs(myNormalMCVSOutput);\n",        // but we add this
    false                                                 // only do it once
  );
}

int TestCompositePolyDataMapper2CustomShader(int argc, char* argv[])
{
  vtkNew<vtkRenderer> renderer;

  // Generate two copies of a vtkPolyData containing the same sphere
  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->Update();
  vtkPolyData* sphere = vtkPolyData::SafeDownCast(sphereSource->GetOutputDataObject(0));

  vtkSmartPointer<vtkPolyData> sphere1 = vtkSmartPointer<vtkPolyData>::Take(sphere->NewInstance());
  sphere1->DeepCopy(sphere);

  sphereSource->SetCenter(1., 0., 0.);
  sphereSource->Update();
  sphere = vtkPolyData::SafeDownCast(sphereSource->GetOutputDataObject(0));

  vtkNew<vtkPolyData> sphere2;
  sphere2->DeepCopy(sphere);

  // Generate scalars with indices for all points on the sphere
  vtkNew<vtkFloatArray> scalars;
  scalars->SetName("Scalars");
  scalars->SetNumberOfComponents(1);
  scalars->SetNumberOfTuples(sphere1->GetNumberOfPoints());
  for (vtkIdType i = 0; i < scalars->GetNumberOfTuples(); ++i)
  {
    scalars->SetTypedComponent(i, 0, static_cast<float>(i));
  }

  // Only add scalars to sphere 1.
  sphere1->GetPointData()->SetScalars(scalars);

  vtkNew<vtkMultiBlockDataSet> mbds;
  mbds->SetNumberOfBlocks(2);
  mbds->SetBlock(0, sphere1);
  mbds->SetBlock(1, sphere2);

  vtkNew<vtkLookupTable> lut;
  lut->SetValueRange(scalars->GetRange());
  lut->SetNanColor(1., 1., 0., 1.);
  lut->Build();

  vtkNew<vtkCompositePolyDataMapper2> mapper;
  mapper->SetInputDataObject(mbds);
  mapper->SetLookupTable(lut);
  mapper->SetScalarVisibility(1);
  mapper->SetScalarRange(scalars->GetRange());
  mapper->SetColorMissingArraysWithNanColor(true);
  mapper->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(0., 0., 1.);
  actor->GetProperty()->SetAmbientColor(0.2, 0.2, 1.0);
  actor->GetProperty()->SetDiffuseColor(1.0, 0.65, 0.7);
  actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetSpecular(0.5);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetAmbient(0.5);
  actor->GetProperty()->SetSpecularPower(20.0);
  actor->GetProperty()->SetOpacity(1.0);
  FillShaderProperty(actor.GetPointer());
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  iren->SetRenderWindow(renWin);
  renWin->AddRenderer(renderer);

  renWin->SetSize(500, 500);
  renderer->GetActiveCamera()->SetPosition(0, 0, 1);
  renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
  renderer->ResetCamera();

  renWin->Render();

  int retVal = vtkRegressionTestImageThreshold(renWin, 15);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
