/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestThreshold.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkDataSetMapper.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkMaskPoints.h"
#include "vtkmLevelOfDetail.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestUtilities.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkTriangleFilter.h"

int TestVTKMLevelOfDetail(int argc, char *argv[])
{
  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderWindowInteractor> iren;

  renWin->AddRenderer(ren.GetPointer());
  iren->SetRenderWindow(renWin.GetPointer());

  //---------------------------------------------------
  // Load file and make only triangles
  //---------------------------------------------------
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/cow.vtp");
  vtkNew<vtkXMLPolyDataReader> reader;
  reader->SetFileName(fname);
  delete[] fname;

  vtkNew<vtkTriangleFilter> clean;
  clean->SetInputConnection(reader->GetOutputPort());
  clean->Update();

  //---------------------------------------------------
  // Test LOD filter 4 times
  // We will setup 4 instances of the filter at different
  // levels of subdivision to make sure it is working properly
  //---------------------------------------------------

  std::vector< vtkNew<vtkmLevelOfDetail> > levelOfDetails(4);
  std::vector< vtkNew<vtkDataSetSurfaceFilter> > surfaces(4);
  std::vector< vtkNew<vtkPolyDataMapper> > mappers(4);
  std::vector< vtkNew<vtkActor> > actors(4);

  for(int i=0; i < 4; ++i)
    {
    levelOfDetails[i]->SetInputConnection(clean->GetOutputPort());
    //subdivision levels of 16, 32, 48, 64
    levelOfDetails[i]->SetNumberOfXDivisions( ((i+1) * 16) );
    levelOfDetails[i]->SetNumberOfYDivisions( ((i+1) * 16) );
    levelOfDetails[i]->SetNumberOfZDivisions( ((i+1) * 16));

    surfaces[i]->SetInputConnection(levelOfDetails[i]->GetOutputPort());

    mappers[i]->SetInputConnection(surfaces[i]->GetOutputPort());

    actors[i]->SetMapper(mappers[i].GetPointer());
    actors[i]->SetPosition( i * 10, 0, 0);

    ren->AddActor(actors[i].GetPointer());
    }

  ren->SetBackground(0.1, 0.2, 0.4);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(6.);
  renWin->SetSize(1600, 250);

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin.GetPointer());
  if(retVal == vtkRegressionTester::DO_INTERACTOR)
  {
  iren->Start();
  retVal = vtkRegressionTester::PASSED;
  }
  return (!retVal);
}
