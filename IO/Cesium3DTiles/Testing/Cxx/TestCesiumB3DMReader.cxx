// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// .NAME Test of an RGBA texture on a vtkActor.
// .SECTION Description
// this program tests the CityGML Reader and setting of textures to
// individual datasets of the multiblock tree.

// VTK_DEPRECATED_IN_9_4_0()
#define VTK_DEPRECATION_LEVEL 0

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCesiumB3DMReader.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkFieldData.h"
#include "vtkGLTFDocumentLoader.h"
#include "vtkGLTFReader.h"
#include "vtkGLTFTexture.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPNGWriter.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"

void AddActors(vtkRenderer* renderer, vtkMultiBlockDataSet* mb, vtkGLTFReader* reader)
{
  using Opts = vtk::DataObjectTreeOptions;
  auto range = vtk::Range(mb, Opts::SkipEmptyNodes | Opts::TraverseSubTree | Opts::VisitOnlyLeaves);
  size_t partitionIndex = 0;
  for (auto o : range)
  {
    vtkPolyData* poly = vtkPolyData::SafeDownCast(o);
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputDataObject(poly);

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
    auto t = reader->GetTexture(partitionIndex);
    auto texture = t->GetVTKTexture();
    // flip texture coordinates
    if (actor->GetPropertyKeys() == nullptr)
    {
      vtkNew<vtkInformation> info;
      actor->SetPropertyKeys(info);
    }
    double mat[] = { 1, 0, 0, 0, 0, -1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1 };
    actor->GetPropertyKeys()->Set(vtkProp::GeneralTextureTransform(), mat, 16);

    actor->SetTexture(texture);
    ++partitionIndex;
  }
}

int TestCesiumB3DMReader(int argc, char* argv[])
{
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/3DTiles/jacksonville-gltf/9/9.glb");

  std::cout << fname << std::endl;
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.5, 0.7, 0.7);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renWin);

  vtkNew<vtkGLTFReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  vtkMultiBlockDataSet* mb = reader->GetOutput();

  AddActors(renderer, mb, reader);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(1.1);

  renWin->SetSize(400, 400);
  renWin->Render();
  interactor->Initialize();
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }

  delete[] fname;
  return !retVal;
}
