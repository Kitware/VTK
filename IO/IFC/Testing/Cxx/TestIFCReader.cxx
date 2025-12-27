// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// .NAME Test of an RGBA texture on a vtkActor.
// .SECTION Description
// this program tests the IFC Reader and setting of textures to
// individual datasets of the multiblock tree.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeDataIterator.h"
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
void AddActors(vtkRenderer* renderer, vtkPartitionedDataSetCollection* pdc, const char* fname)
{
  vtkSmartPointer<vtkCompositeDataIterator> it;
  for (it.TakeReference(pdc->NewIterator()); !it->IsDoneWithTraversal(); it->GoToNextItem())
  {
    vtkPolyData* poly = vtkPolyData::SafeDownCast(it->GetCurrentDataObject());
    if (poly)
    {
      auto diffuse = vtkPolyDataMaterial::GetField(
        poly, vtkPolyDataMaterial::DIFFUSE_COLOR, std::vector<double>{ 1, 1, 1 });
      auto specular = vtkPolyDataMaterial::GetField(
        poly, vtkPolyDataMaterial::SPECULAR_COLOR, std::vector<double>{ 1, 1, 1 });
      double shininess = vtkPolyDataMaterial::GetField(
        poly, vtkPolyDataMaterial::SHININESS, std::vector<double>{ 0.5 })[0];
      double transparency = vtkPolyDataMaterial::GetField(
        poly, vtkPolyDataMaterial::TRANSPARENCY, std::vector<double>{ 0.5 })[0];
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

  std::cout << fname << std::endl;
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.5, 0.7, 0.7);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renWin);

  vtkNew<vtkIFCReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  vtkPartitionedDataSetCollection* pdc = reader->GetOutput();

  ::AddActors(renderer, pdc, fname);

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
