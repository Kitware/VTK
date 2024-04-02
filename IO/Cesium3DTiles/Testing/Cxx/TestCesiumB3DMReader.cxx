// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// .NAME Test of an RGBA texture on a vtkActor.
// .SECTION Description
// this program tests the CityGML Reader and setting of textures to
// individual datasets of the multiblock tree.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCesiumB3DMReader.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkFieldData.h"
#include "vtkGLTFDocumentLoader.h"
#include "vtkGLTFReader.h"
#include "vtkImageData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"

vtkSmartPointer<vtkTexture> CreateVTKTextureFromGLTFTexture(vtkGLTFReader::GLTFTexture& gltfTexture)
{
  vtkNew<vtkTexture> texture;
  texture->SetColorModeToDirectScalars();
  texture->SetBlendingMode(vtkTexture::VTK_TEXTURE_BLENDING_MODE_MODULATE);
  // Approximate filtering settings
  if (gltfTexture.MinFilterValue == vtkGLTFDocumentLoader::Sampler::FilterType::NEAREST ||
    gltfTexture.MinFilterValue == vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR)
  {
    texture->MipmapOff();
  }
  else
  {
    texture->MipmapOn();
  }

  if (gltfTexture.WrapSValue == vtkGLTFDocumentLoader::Sampler::WrapType::CLAMP_TO_EDGE ||
    gltfTexture.WrapTValue == vtkGLTFDocumentLoader::Sampler::WrapType::CLAMP_TO_EDGE)
  {
    texture->RepeatOff();
    texture->EdgeClampOn();
  }
  else if (gltfTexture.WrapSValue == vtkGLTFDocumentLoader::Sampler::WrapType::REPEAT ||
    gltfTexture.WrapTValue == vtkGLTFDocumentLoader::Sampler::WrapType::REPEAT)
  {
    texture->RepeatOn();
    texture->EdgeClampOff();
  }
  else
  {
    vtkWarningWithObjectMacro(nullptr, "Mirrored texture wrapping is not supported!");
  }

  if (gltfTexture.MinFilterValue == vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR ||
    gltfTexture.MinFilterValue ==
      vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR_MIPMAP_NEAREST ||
    gltfTexture.MinFilterValue ==
      vtkGLTFDocumentLoader::Sampler::FilterType::NEAREST_MIPMAP_LINEAR ||
    gltfTexture.MinFilterValue ==
      vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR_MIPMAP_LINEAR ||
    gltfTexture.MaxFilterValue == vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR ||
    gltfTexture.MaxFilterValue ==
      vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR_MIPMAP_NEAREST ||
    gltfTexture.MaxFilterValue ==
      vtkGLTFDocumentLoader::Sampler::FilterType::NEAREST_MIPMAP_LINEAR ||
    gltfTexture.MaxFilterValue == vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR_MIPMAP_LINEAR)
  {
    texture->InterpolateOn();
  }

  texture->SetInputData(gltfTexture.Image);
  return texture;
}

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
    vtkGLTFReader::GLTFTexture t = reader->GetGLTFTexture(partitionIndex);
    auto texture = CreateVTKTextureFromGLTFTexture(t);
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
  renderer->GetActiveCamera()->Azimuth(90);
  renderer->GetActiveCamera()->Roll(-90);
  renderer->GetActiveCamera()->Zoom(1.5);

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
