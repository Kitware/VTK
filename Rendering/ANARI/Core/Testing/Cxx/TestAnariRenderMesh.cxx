// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkActorCollection.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkExtractEdges.h"
#include "vtkImageData.h"
#include "vtkLight.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkStringScanner.h"
#include "vtkStripper.h"
#include "vtkTexture.h"
#include "vtkTextureMapToSphere.h"
#include "vtkTransformTextureCoords.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVertexGlyphFilter.h"

#include "vtkAnariSceneGraph.h"
#include "vtkAnariTestUtilities.h"

namespace
{

struct Renderable
{
  vtkNew<vtkSphereSource> Sphere;
  vtkNew<vtkPolyDataMapper> Mapper;
  vtkNew<vtkActor> Actor;
};

std::unique_ptr<Renderable> MakeSphereAt(double x, double y, double z, int res, int type, int rep)
{
  std::unique_ptr<Renderable> ret = std::make_unique<Renderable>();
  ret->Sphere->SetEndTheta(180); // half spheres better show variation and f and back
  ret->Sphere->SetStartPhi(30);
  ret->Sphere->SetEndPhi(150);
  ret->Sphere->SetPhiResolution(res);
  ret->Sphere->SetThetaResolution(res);
  ret->Sphere->SetCenter(x, y, z);

  // make texture coordinate
  vtkNew<vtkTextureMapToSphere> tc;
  tc->SetCenter(x, y, z);
  tc->PreventSeamOn();
  tc->AutomaticSphereGenerationOff();
  tc->SetInputConnection(ret->Sphere->GetOutputPort());
  vtkNew<vtkTransformTextureCoords> tt;
  tt->SetInputConnection(tc->GetOutputPort());

  // make normals
  vtkNew<vtkPolyDataNormals> nl;
  nl->SetInputConnection(tt->GetOutputPort());
  nl->Update();

  // make more attribute arrays
  vtkPolyData* pd = nl->GetOutput();

  // point aligned
  vtkNew<vtkDoubleArray> da1;
  da1->SetName("testarray1");
  da1->SetNumberOfComponents(1);
  pd->GetPointData()->AddArray(da1);
  int np = pd->GetNumberOfPoints();
  int nc = pd->GetNumberOfCells();
  for (int i = 0; i < np; i++)
  {
    da1->InsertNextValue((double)i / (double)np);
  }
  vtkNew<vtkDoubleArray> da2;
  da2->SetName("testarray2");
  da2->SetNumberOfComponents(3);
  pd->GetPointData()->AddArray(da2);
  for (int i = 0; i < np; i++)
  {
    double vals[3] = { (double)i / (double)np, (double)(i * 4) / (double)np - 2.0, 42.0 };
    da2->InsertNextTuple3(vals[0], vals[1], vals[2]);
  }
  vtkNew<vtkUnsignedCharArray> pac;
  pac->SetName("testarrayc1");
  pac->SetNumberOfComponents(3);
  pd->GetPointData()->AddArray(pac);
  for (int i = 0; i < np; i++)
  {
    unsigned char vals[3] = { static_cast<unsigned char>(255 * ((double)i / (double)np)),
      static_cast<unsigned char>(255 * ((double)(i * 4) / (double)np - 2.0)), 42 };
    pac->InsertNextTuple3(vals[0], vals[1], vals[2]);
  }
  vtkNew<vtkUnsignedCharArray> ca1;
  ca1->SetName("testarray3");
  ca1->SetNumberOfComponents(3);
  pd->GetPointData()->AddArray(ca1);
  for (int i = 0; i < np; i++)
  {
    unsigned char vals[3] = { static_cast<unsigned char>((double)i / (double)np * 255),
      static_cast<unsigned char>((double)(1 - i) / (double)np), 42 };
    ca1->InsertNextTuple3(vals[0], vals[1], vals[2]);
  }
  // cell aligned
  vtkNew<vtkDoubleArray> da4;
  da4->SetName("testarray4");
  da4->SetNumberOfComponents(1);
  pd->GetCellData()->AddArray(da4);
  for (int i = 0; i < pd->GetNumberOfCells(); i++)
  {
    da4->InsertNextValue((double)i / (double)pd->GetNumberOfCells());
  }
  vtkNew<vtkDoubleArray> da5;
  da5->SetName("testarray5");
  da5->SetNumberOfComponents(3);
  pd->GetCellData()->AddArray(da5);
  for (int i = 0; i < nc; i++)
  {
    double vals[3] = { (double)i / (double)nc, (double)(i * 2) / (double)nc, 42.0 };
    da5->InsertNextTuple3(vals[0], vals[1], vals[2]);
  }
  vtkNew<vtkUnsignedCharArray> ca2;
  ca2->SetName("testarray6");
  ca2->SetNumberOfComponents(3);
  pd->GetCellData()->AddArray(ca2);
  for (int i = 0; i < nc; i++)
  {
    unsigned char vals[3] = { static_cast<unsigned char>((double)i / (double)np * 255),
      static_cast<unsigned char>((double)(1 - i) / (double)np), 42 };
    ca2->InsertNextTuple3(vals[0], vals[1], vals[2]);
  }
  ret->Mapper->SetInputData(pd);

  switch (type)
  {
    case 0: // points
    {
      vtkNew<vtkVertexGlyphFilter> filter;
      filter->SetInputData(pd);
      filter->Update();
      ret->Mapper->SetInputData(filter->GetOutput());
      break;
    }
    case 1: // lines
    {
      vtkNew<vtkExtractEdges> filter;
      filter->SetInputData(pd);
      filter->Update();
      ret->Mapper->SetInputData(filter->GetOutput());
      break;
    }
    case 2: // polys
      break;
    case 3: // strips
    {
      vtkNew<vtkStripper> filter;
      filter->SetInputData(pd);
      filter->Update();
      ret->Mapper->SetInputData(filter->GetOutput());
      break;
    }
  }
  ret->Actor->SetMapper(ret->Mapper);
  ret->Actor->GetProperty()->SetPointSize(20.0f);
  ret->Actor->GetProperty()->SetLineWidth(1.0f);
  if (rep != -1)
  {
    ret->Actor->GetProperty()->SetRepresentation(rep);
  }
  return ret;
}

}

/**
 * This test verifies that we can do simple mesh rendering with ANARI
 * and that VTK's many standard rendering modes (points, lines, surface, with
 * a variety of color controls (actor, point, cell, texture) etc work as
 * they should.
 *
 * The command line arguments are:
 * -type N   => where N is one of 0,1,2, or 3 makes meshes consisting of
 *              points, wireframes, triangles (=the default) or triangle strips
 * -rep N    => where N is one of 0,1 or 2 draws the meshes as points, lines
 *              or surfaces
 */
int TestAnariRenderMesh(int argc, char* argv[])
{
  bool useDebugDevice = false;
  int type = 2;
  int rep = -1;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "--type"))
    {
      VTK_FROM_CHARS_IF_ERROR_RETURN(argv[i + 1], type, EXIT_FAILURE);
    }
    else if (!strcmp(argv[i], "--rep"))
    {
      VTK_FROM_CHARS_IF_ERROR_RETURN(argv[i + 1], rep, EXIT_FAILURE);
    }
    else if (!strcmp(argv[i], "--trace"))
    {
      useDebugDevice = true;
    }
  }

  vtkNew<vtkRenderer> renderer;
  renderer->AutomaticLightCreationOn();
  renderer->SetBackground(0.75, 0.75, 0.75);
  vtkNew<vtkCamera> camera;
  camera->SetPosition(2.5, 11, -3);
  camera->SetFocalPoint(2.5, 0, -3);
  camera->SetViewUp(0, 0, 1);
  renderer->SetActiveCamera(camera);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetSize(600, 550);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkAnariTestUtilities::SetParameterDefaults(renWin, useDebugDevice, "TestAnariRendererMesh");

  // Now, vary most of the many parameters that rendering can vary by.

  // representations points, wireframe, surface
  std::unique_ptr<::Renderable> ren = ::MakeSphereAt(5, 0, -5, 10, type, 0);
  renderer->AddActor(ren->Actor);

  ren = ::MakeSphereAt(5, 0, -4, 10, 1, 1);
  ren->Actor->GetProperty()->SetColor(1, 0, 0);
  renderer->AddActor(ren->Actor);

  ren = ::MakeSphereAt(5, 0, -3, 10, type, rep);
  ren->Actor->GetProperty()->SetRepresentationToSurface();
  renderer->AddActor(ren->Actor);

  // actor color
  ren = ::MakeSphereAt(4, 0, -5, 10, type, rep);
  ren->Actor->GetProperty()->SetColor(0, 1, 0);
  renderer->AddActor(ren->Actor);

  // ambient, diffuse, and specular components
  ren = ::MakeSphereAt(4, 0, -4, 7, type, rep);
  ren->Actor->GetProperty()->SetAmbient(0.5);
  ren->Actor->GetProperty()->SetAmbientColor(0.1, 0.1, 0.3);
  ren->Actor->GetProperty()->SetDiffuse(0.4);
  ren->Actor->GetProperty()->SetDiffuseColor(0.5, 0.1, 0.1);
  ren->Actor->GetProperty()->SetSpecular(0.2);
  ren->Actor->GetProperty()->SetSpecularColor(1, 1, 1);
  ren->Actor->GetProperty()->SetSpecularPower(100);
  ren->Actor->GetProperty()->SetInterpolationToPhong();
  renderer->AddActor(ren->Actor);

  // opacity
  ren = ::MakeSphereAt(4, 0, -3, 10, type, rep);
  ren->Actor->GetProperty()->SetOpacity(0.2);
  renderer->AddActor(ren->Actor);

  // color map cell values
  ren = ::MakeSphereAt(3, 0, -5, 10, type, rep);
  ren->Mapper->SetScalarModeToUseCellFieldData();
  ren->Mapper->SelectColorArray(0);
  renderer->AddActor(ren->Actor);

  // default color component
  ren = ::MakeSphereAt(3, 0, -4, 10, type, rep);
  ren->Mapper->SetScalarModeToUseCellFieldData();
  ren->Mapper->SelectColorArray(1);
  renderer->AddActor(ren->Actor);

  // choose color component
  ren = ::MakeSphereAt(3, 0, -3, 10, type, rep);
  ren->Mapper->SetScalarModeToUseCellFieldData();
  ren->Mapper->SelectColorArray(1);
  ren->Mapper->ColorByArrayComponent(1, 1); // todo, use lut since this is deprecated
  renderer->AddActor(ren->Actor);

  // RGB direct
  ren = ::MakeSphereAt(3, 0, -2, 10, type, rep);
  ren->Mapper->SetScalarModeToUseCellFieldData();
  ren->Mapper->SelectColorArray(2);
  renderer->AddActor(ren->Actor);

  // RGB through LUT
  ren = ::MakeSphereAt(3, 0, -1, 10, type, rep);
  ren->Mapper->SetScalarModeToUseCellFieldData();
  ren->Mapper->SelectColorArray(2);
  ren->Mapper->SetColorModeToMapScalars();
  renderer->AddActor(ren->Actor);

  // color map point values
  ren = ::MakeSphereAt(2, 0, -5, 6, type, rep);
  ren->Mapper->SetScalarModeToUsePointFieldData();
  ren->Mapper->SelectColorArray("testarray1");
  renderer->AddActor(ren->Actor);

  // interpolate scalars before mapping
  ren = ::MakeSphereAt(2, 0, -4, 6, type, rep);
  ren->Mapper->SetScalarModeToUsePointFieldData();
  ren->Mapper->SelectColorArray("testarray1");
  ren->Mapper->InterpolateScalarsBeforeMappingOn();
  renderer->AddActor(ren->Actor);

  // RGB direct
  ren = ::MakeSphereAt(2, 0, -3, 10, type, rep);
  ren->Mapper->SetScalarModeToUsePointFieldData();
  ren->Mapper->SetColorModeToDefault();
  ren->Mapper->SelectColorArray("testarrayc1");
  renderer->AddActor(ren->Actor);

  // RGB mapped
  ren = ::MakeSphereAt(2, 0, -2, 10, type, rep);
  ren->Mapper->SetScalarModeToUsePointFieldData();
  ren->Mapper->SetColorModeToMapScalars();
  ren->Mapper->SelectColorArray("testarrayc1");
  renderer->AddActor(ren->Actor);

  // unlit, flat, and gouraud lighting
  ren = ::MakeSphereAt(1, 0, -5, 7, type, rep);
  ren->Actor->GetProperty()->LightingOff();
  renderer->AddActor(ren->Actor);

  ren = ::MakeSphereAt(1, 0, -4, 7, type, rep);
  ren->Actor->GetProperty()->SetInterpolationToFlat();
  renderer->AddActor(ren->Actor);

  ren = ::MakeSphereAt(1, 0, -3, 7, type, rep);
  ren->Actor->GetProperty()->SetInterpolationToGouraud();
  renderer->AddActor(ren->Actor);

  // texture
  int maxi = 100;
  int maxj = 100;
  vtkNew<vtkImageData> texin;
  texin->SetExtent(0, maxi, 0, maxj, 0, 0);
  texin->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
  vtkUnsignedCharArray* aa =
    vtkArrayDownCast<vtkUnsignedCharArray>(texin->GetPointData()->GetScalars());
  int idx = 0;
  for (int i = 0; i <= maxi; i++)
  {
    for (int j = 0; j <= maxj; j++)
    {
      bool ival = (i / 10) % 2 == 1;
      bool jval = (j / 10) % 2 == 1;
      unsigned char val = (ival ^ jval) ? 255 : 0;
      aa->SetTuple3(idx, val, val, val);
      if (j <= 3 || j >= maxj - 3)
      {
        aa->SetTuple3(idx, 255, 255, 0);
      }
      if (i <= 20 || i >= maxi - 20)
      {
        aa->SetTuple3(idx, 255, 0, 0);
      }
      idx = idx + 1;
    }
  }
  ren = ::MakeSphereAt(0, 0, -5, 20, type, rep);
  renderer->AddActor(ren->Actor);
  vtkNew<vtkTexture> texture;
  texture->SetInputData(texin);
  ren->Actor->SetTexture(texture);

  // imagespace positional transformations
  ren = ::MakeSphereAt(0, 0, -4, 10, type, rep);
  ren->Actor->SetScale(1.2, 1.0, 0.87);
  renderer->AddActor(ren->Actor);

  int retVal = vtkRegressionTestImage(renWin);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
