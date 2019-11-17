/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCityGMLReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Test of an RGBA texture on a vtkActor.
// .SECTION Description
// this program tests the CityGML Reader and setting of textures to
// individual datasets of the multiblock tree.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCityGMLReader.h"
#include "vtkCompositeDataIterator.h"
#include "vtkFieldData.h"
#include "vtkJPEGReader.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStringArray.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"
#include "vtksys/SystemTools.hxx"

int TestCityGMLReader(int argc, char* argv[])
{
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/CityGML/Part-4-Buildings-V4-one.gml");

  std::cout << fname << std::endl;
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.5, 0.7, 0.7);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renWin);

  vtkNew<vtkCityGMLReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  vtkMultiBlockDataSet* mb = reader->GetOutput();

  vtkSmartPointer<vtkCompositeDataIterator> it;
  for (it.TakeReference(mb->NewIterator()); !it->IsDoneWithTraversal(); it->GoToNextItem())
  {
    vtkPolyData* poly = vtkPolyData::SafeDownCast(it->GetCurrentDataObject());
    if (poly)
    {

      vtkNew<vtkPolyDataMapper> mapper;
      mapper->SetInputDataObject(poly);

      vtkNew<vtkActor> actor;
      actor->SetMapper(mapper);
      renderer->AddActor(actor);
      vtkStringArray* textureField =
        vtkStringArray::SafeDownCast(poly->GetFieldData()->GetAbstractArray("texture_uri"));
      if (textureField)
      {
        std::string fnamePath = vtksys::SystemTools::GetFilenamePath(std::string(fname));

        const char* textureURI = textureField->GetValue(0);
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
