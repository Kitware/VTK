/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMultiBlock.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example demonstrates how hierarchical box (uniform rectilinear)
// AMR datasets can be processed using the new vtkHierarchicalBoxDataSet class. 
// 
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
// -D <path> => path to the data; the data should be in <path>/Data/

#include "vtkCellDataToPointData.h"
#include "vtkContourFilter.h"
#include "vtkDebugLeaks.h"
#include "vtkMultiBlockDataSetGeometryFilter.h"
#include "vtkOutlineCornerFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkShrinkPolyData.h"
#include "vtkTestMultiBlockDataReader.h"
#include "vtkTestUtilities.h"

int TestMultiBlock(int argc, char* argv[])
{
  // Disable for testing
  vtkDebugLeaks::PromptUserOff();

  // Standard rendering classes
  vtkRenderer *ren = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  char* cfname = 
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/multicomb");

  vtkTestMultiBlockDataReader* reader = vtkTestMultiBlockDataReader::New();
  reader->SetFileName(cfname);
  delete[] cfname;

  // geometry filter
  vtkMultiBlockDataSetGeometryFilter* geom = 
    vtkMultiBlockDataSetGeometryFilter::New();
  geom->SetInputConnection(0, reader->GetOutputPort(0));

  vtkShrinkPolyData* shrink = vtkShrinkPolyData::New();
  shrink->SetShrinkFactor(0.2);
  shrink->SetInput(geom->GetOutput());

  // Rendering objects
  vtkPolyDataMapper* shMapper = vtkPolyDataMapper::New();
  shMapper->SetInput(shrink->GetOutput());
  vtkActor* shActor = vtkActor::New();
  shActor->SetMapper(shMapper);
  shActor->GetProperty()->SetColor(0, 0, 1);
  ren->AddActor(shActor);

  // corner outline
  vtkOutlineCornerFilter* ocf = vtkOutlineCornerFilter::New();
  ocf->SetInputConnection(0, reader->GetOutputPort(0));

  // geometry filter
  vtkMultiBlockDataSetGeometryFilter* geom2 = 
    vtkMultiBlockDataSetGeometryFilter::New();
  geom2->SetInputConnection(0, ocf->GetOutputPort(0));

  // Rendering objects
  vtkPolyDataMapper* ocMapper = vtkPolyDataMapper::New();
  ocMapper->SetInput(geom2->GetOutput());
  vtkActor* ocActor = vtkActor::New();
  ocActor->SetMapper(ocMapper);
  ocActor->GetProperty()->SetColor(1, 0, 0);
  ren->AddActor(ocActor);

  // cell 2 point and contour
  vtkCellDataToPointData* c2p = vtkCellDataToPointData::New();
  c2p->SetInputConnection(0, reader->GetOutputPort(0));

  vtkContourFilter* contour = vtkContourFilter::New();
  contour->SetInputConnection(0, c2p->GetOutputPort(0));
  contour->SetValue(0, 0.45);

  // geometry filter
  vtkMultiBlockDataSetGeometryFilter* geom3 = 
    vtkMultiBlockDataSetGeometryFilter::New();
  geom3->SetInputConnection(0, contour->GetOutputPort(0));

  // Rendering objects
  vtkPolyDataMapper* contMapper = vtkPolyDataMapper::New();
  contMapper->SetInput(geom3->GetOutput());
  vtkActor* contActor = vtkActor::New();
  contActor->SetMapper(contMapper);
  contActor->GetProperty()->SetColor(1, 0, 0);
  ren->AddActor(contActor);
  
  // Standard testing code.
  ocf->Delete();
  geom2->Delete();
  ocMapper->Delete();
  ocActor->Delete();
  c2p->Delete();
  contour->Delete();
  geom3->Delete();
  contMapper->Delete();
  contActor->Delete();
  ren->SetBackground(1,1,1);
  renWin->SetSize(300,300);
  renWin->Render();
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }
  
  // Cleanup
  geom->Delete();
  shMapper->Delete();
  shActor->Delete();
  ren->Delete();
  renWin->Delete();
  iren->Delete();
  reader->Delete();
  shrink->Delete();
  
  return !retVal;
}
