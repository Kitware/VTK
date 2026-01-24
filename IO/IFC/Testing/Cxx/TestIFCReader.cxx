// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// .NAME Test of an RGBA texture on a vtkActor.
// .SECTION Description
// this program tests the IFC Reader and setting of textures to
// individual datasets of the multiblock tree.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkFieldData.h"
#include "vtkIFCReader.h"
#include "vtkJPEGReader.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataMaterial.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStringArray.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"
#include "vtksys/SystemTools.hxx"

#include <iostream>

namespace
{
void AddCompositePolyDataMapper(
  vtkRenderer* renderer, vtkPartitionedDataSetCollection* pdc, const char* fname)
{
  vtkNew<vtkCompositePolyDataMapper> mapper;
  vtkNew<vtkCompositeDataDisplayAttributes> attrs;
  mapper->SetCompositeDataDisplayAttributes(attrs);

  mapper->SetInputDataObject(pdc);
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);
  vtkSmartPointer<vtkCompositeDataIterator> it;
  for (it.TakeReference(pdc->NewIterator()); !it->IsDoneWithTraversal(); it->GoToNextItem())
  {
    unsigned int flatIndex = it->GetCurrentFlatIndex();
    vtkPolyData* poly = vtkPolyData::SafeDownCast(it->GetCurrentDataObject());
    if (poly)
    {
      auto diffuse = vtkPolyDataMaterial::GetField(
        poly, vtkPolyDataMaterial::GetDiffuseColorName(), std::vector<double>{ 1, 1, 1 });
      auto specular = vtkPolyDataMaterial::GetField(
        poly, vtkPolyDataMaterial::GetSpecularColorName(), std::vector<double>{ 1, 1, 1 });
      double shininess = vtkPolyDataMaterial::GetField(
        poly, vtkPolyDataMaterial::GetShininessName(), std::vector<double>{ 1 })[0];
      double transparency = vtkPolyDataMaterial::GetField(
        poly, vtkPolyDataMaterial::GetTransparencyName(), std::vector<double>{ 0 })[0];
      mapper->SetBlockColor(flatIndex, diffuse.data());
      mapper->SetBlockOpacity(flatIndex, 1 - transparency);

      vtkStringArray* textureField =
        vtkStringArray::SafeDownCast(poly->GetFieldData()->GetAbstractArray("texture_uri"));
      if (textureField)
      {
        std::string fnamePath = vtksys::SystemTools::GetFilenamePath(std::string(fname));

        vtkStdString textureURI = textureField->GetValue(0);
        vtkNew<vtkJPEGReader> JpegReader;
        JpegReader->SetFileName((fnamePath + "/" + textureURI).c_str());
        JpegReader->Update();

        vtkNew<vtkTexture> texture;
        texture->SetInputConnection(JpegReader->GetOutputPort());
        texture->InterpolateOn();

        mapper->SetBlockTexture(flatIndex, texture);
      }
    }
  }
}

void AddPolyDataMappers(
  vtkRenderer* renderer, vtkPartitionedDataSetCollection* pdc, const char* fname)
{
  vtkSmartPointer<vtkCompositeDataIterator> it;
  for (it.TakeReference(pdc->NewIterator()); !it->IsDoneWithTraversal(); it->GoToNextItem())
  {
    vtkPolyData* poly = vtkPolyData::SafeDownCast(it->GetCurrentDataObject());
    if (poly)
    {
      auto diffuse = vtkPolyDataMaterial::GetField(
        poly, vtkPolyDataMaterial::GetDiffuseColorName(), std::vector<double>{ 1, 1, 1 });
      auto specular = vtkPolyDataMaterial::GetField(
        poly, vtkPolyDataMaterial::GetSpecularColorName(), std::vector<double>{ 1, 1, 1 });
      double shininess = vtkPolyDataMaterial::GetField(
        poly, vtkPolyDataMaterial::GetShininessName(), std::vector<double>{ 1 })[0];
      double transparency = vtkPolyDataMaterial::GetField(
        poly, vtkPolyDataMaterial::GetTransparencyName(), std::vector<double>{ 0 })[0];
      vtkNew<vtkPolyDataMapper> mapper;
      mapper->SetInputDataObject(poly);

      vtkNew<vtkActor> actor;
      vtkProperty* property = actor->GetProperty();
      property->SetDiffuseColor(diffuse.data());
      property->SetSpecularColor(specular.data());
      property->SetSpecularPower(shininess);
      property->SetOpacity(1 - transparency);
      actor->SetMapper(mapper);
      renderer->AddActor(actor);
      vtkStringArray* textureField =
        vtkStringArray::SafeDownCast(poly->GetFieldData()->GetAbstractArray("texture_uri"));
      if (textureField)
      {
        std::string fnamePath = vtksys::SystemTools::GetFilenamePath(std::string(fname));

        vtkStdString textureURI = textureField->GetValue(0);
        vtkNew<vtkJPEGReader> JpegReader;
        JpegReader->SetFileName((fnamePath + "/" + textureURI).c_str());
        JpegReader->Update();

        vtkNew<vtkTexture> texture;
        texture->SetInputConnection(JpegReader->GetOutputPort());
        texture->InterpolateOn();

        actor->SetTexture(texture);
      }
    }
  }
}
}

int TestIFCReader(int argc, char* argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/IFC/duplex.ifc");
  if (argc >= 9)
  {
    // debug code to be able to load another datafile
    size_t len = strlen(argv[argc - 1]);
    fname = new char[len + 1];
    std::copy(argv[argc - 1], argv[argc - 1] + len + 1, fname);
  }

  std::cout << fname << std::endl;
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.5, 0.7, 0.7);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renWin);

  vtkNew<vtkIFCReader> reader;
  reader->SetFileName(fname);
  reader->IncludeCurvesOn();
  reader->Update();
  vtkPartitionedDataSetCollection* pdc = reader->GetOutput();

  ::AddCompositePolyDataMapper(renderer, pdc, fname);
  //::AddPolyDataMappers(renderer, pdc, fname);

  renderer->GetActiveCamera()->Elevation(-80);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(2);

  renWin->SetSize(400, 400);
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
