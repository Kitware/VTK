// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test covers the vtkPolyDataTangents filter

#include "vtkActor.h"
#include "vtkActorCollection.h"
#include "vtkArrowSource.h"
#include "vtkCamera.h"
#include "vtkDataSetAttributes.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkGlyph3DMapper.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkJPEGReader.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkPolyDataTangents.h"
#include "vtkProperty.h"
#include "vtkRandomAttributeGenerator.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"
#include "vtkTextureMapToCylinder.h"
#include "vtkTriangleFilter.h"
#include "vtkXMLPolyDataReader.h"

//------------------------------------------------------------------------------
int TestPolyDataTangents(int argc, char* argv[])
{
  vtkNew<vtkXMLPolyDataReader> reader;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/cow.vtp");
  reader->SetFileName(fname);
  delete[] fname;

  vtkNew<vtkRandomAttributeGenerator> randomAttributes;
  randomAttributes->SetInputConnection(reader->GetOutputPort());
  randomAttributes->GenerateAllDataOff();
  randomAttributes->GenerateCellScalarsOn();

  vtkNew<vtkPolyDataNormals> normals;
  normals->SetInputConnection(randomAttributes->GetOutputPort());
  normals->SplittingOff();

  vtkNew<vtkTriangleFilter> triangle;
  triangle->SetInputConnection(normals->GetOutputPort());

  vtkNew<vtkTextureMapToCylinder> textureMap;
  textureMap->SetInputConnection(triangle->GetOutputPort());

  vtkNew<vtkPolyDataTangents> tangents;
  tangents->SetInputConnection(textureMap->GetOutputPort());

  vtkNew<vtkArrowSource> arrow;
  arrow->SetTipResolution(20);
  arrow->SetShaftResolution(20);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(tangents->GetOutputPort());

  vtkNew<vtkGlyph3DMapper> tgtsMapper;
  tgtsMapper->SetInputConnection(tangents->GetOutputPort());
  tgtsMapper->SetOrientationArray(vtkDataSetAttributes::TANGENTS);
  tgtsMapper->SetSourceConnection(arrow->GetOutputPort());
  tgtsMapper->SetScaleFactor(0.5);

  vtkNew<vtkJPEGReader> image;
  char* texname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/tex_debug.jpg");
  image->SetFileName(texname);
  delete[] texname;

  vtkNew<vtkTexture> texture;
  texture->SetInputConnection(image->GetOutputPort());

  vtkNew<vtkRenderer> renderer;

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->SetTexture(texture);

  vtkNew<vtkActor> actorTangents;
  actorTangents->SetMapper(tgtsMapper);
  actorTangents->GetProperty()->SetColor(1.0, 0.0, 0.0);

  renderer->AddActor(actor);
  renderer->AddActor(actorTangents);

  renWin->Render();

  renderer->GetActiveCamera()->Zoom(3.0);
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
