/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHierarchicalBoxPipeline.cxx

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

#include "vtkCamera.h"
#include "vtkCellDataToPointData.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkContourFilter.h"
#include "vtkDebugLeaks.h"
#include "vtkHierarchicalDataExtractLevel.h"
#include "vtkHierarchicalDataSetGeometryFilter.h"
#include "vtkOutlineCornerFilter.h"
#include "vtkHierarchicalPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkShrinkPolyData.h"
#include "vtkTestUtilities.h"
#include "vtkXMLHierarchicalBoxDataReader.h"

int TestHierarchicalBoxPipeline(int argc, char* argv[])
{
  vtkCompositeDataPipeline* prototype = vtkCompositeDataPipeline::New();
  vtkAlgorithm::SetDefaultExecutivePrototype(prototype);
  prototype->Delete();

  // Standard rendering classes
  vtkRenderer *ren = vtkRenderer::New();
  vtkCamera* cam = ren->GetActiveCamera();
  cam->SetPosition(-5.1828, 5.89733, 8.97969);
  cam->SetFocalPoint(14.6491, -2.08677, -8.92362);
  cam->SetViewUp(0.210794, 0.95813, -0.193784);

  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  char* cfname =
    vtkTestUtilities::ExpandDataFileName(argc, argv,
                                         "Data/chombo3d/chombo3d.vtm");

  vtkXMLHierarchicalBoxDataReader* reader =
    vtkXMLHierarchicalBoxDataReader::New();
  reader->SetFileName(cfname);
  delete[] cfname;

  // geometry filter
  vtkHierarchicalDataSetGeometryFilter* geom =
    vtkHierarchicalDataSetGeometryFilter::New();
  geom->SetInputConnection(0, reader->GetOutputPort(0));

  vtkShrinkPolyData* shrink = vtkShrinkPolyData::New();
  shrink->SetShrinkFactor(0.5);
  shrink->SetInputConnection(0, geom->GetOutputPort(0));

  // Rendering objects
  vtkHierarchicalPolyDataMapper* shMapper = vtkHierarchicalPolyDataMapper::New();
  shMapper->SetInputConnection(0, shrink->GetOutputPort(0));
  vtkActor* shActor = vtkActor::New();
  shActor->SetMapper(shMapper);
  shActor->GetProperty()->SetColor(0, 0, 1);
  ren->AddActor(shActor);

  // corner outline
  vtkOutlineCornerFilter* ocf = vtkOutlineCornerFilter::New();
  ocf->SetInputConnection(0, reader->GetOutputPort(0));

  // Rendering objects
  // This one is actually just a vtkPolyData so it doesn't need a hierarchical
  // mapper, but we use this one to test hierarchical mapper with polydata input
  vtkHierarchicalPolyDataMapper* ocMapper = vtkHierarchicalPolyDataMapper::New();
  ocMapper->SetInputConnection(0, ocf->GetOutputPort(0));
  vtkActor* ocActor = vtkActor::New();
  ocActor->SetMapper(ocMapper);
  ocActor->GetProperty()->SetColor(1, 0, 0);
  ren->AddActor(ocActor);

  // cell 2 point and contour
  vtkHierarchicalDataExtractLevel* el = vtkHierarchicalDataExtractLevel::New();
  el->SetInputConnection(0, reader->GetOutputPort(0));
  el->AddLevel(2);

  vtkCellDataToPointData* c2p = vtkCellDataToPointData::New();
  c2p->SetInputConnection(0, el->GetOutputPort(0));

  vtkContourFilter* contour = vtkContourFilter::New();
  contour->SetInputConnection(0, c2p->GetOutputPort(0));
  contour->SetValue(0, -0.013);
  contour->SetInputArrayToProcess(
    0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,"phi");

  // Rendering objects
  vtkHierarchicalPolyDataMapper* contMapper = vtkHierarchicalPolyDataMapper::New();
  contMapper->SetInputConnection(0, contour->GetOutputPort(0));
  vtkActor* contActor = vtkActor::New();
  contActor->SetMapper(contMapper);
  contActor->GetProperty()->SetColor(1, 0, 0);
  ren->AddActor(contActor);

  // Standard testing code.
  ocf->Delete();
  ocMapper->Delete();
  ocActor->Delete();
  c2p->Delete();
  contour->Delete();
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
  el->Delete();
  geom->Delete();
  shMapper->Delete();
  shActor->Delete();
  ren->Delete();
  renWin->Delete();
  iren->Delete();
  reader->Delete();
  shrink->Delete();

  vtkAlgorithm::SetDefaultExecutivePrototype(0);
  return !retVal;
}
