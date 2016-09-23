/*=========================================================================

  Program:   Visualization Toolkit
  Module:    MultiBlock.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example demonstrates how multi-block datasets can be processed
// using the new vtkMultiBlockDataSet class.
//
// The command line arguments are:
// -D <path> => path to the data (VTKData); the data should be in <path>/Data/

#include "vtkActor.h"
#include "vtkCellDataToPointData.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkContourFilter.h"
#include "vtkDebugLeaks.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkOutlineCornerFilter.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkShrinkPolyData.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridOutlineFilter.h"
#include "vtkTestUtilities.h"
#include "vtkXMLStructuredGridReader.h"
#include <sstream>

int main(int argc, char* argv[])
{
  vtkCompositeDataPipeline* exec = vtkCompositeDataPipeline::New();
  vtkAlgorithm::SetDefaultExecutivePrototype(exec);
  exec->Delete();

  // Standard rendering classes
  vtkRenderer *ren = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // We will read three files and collect them together in one
  // multi-block dataset. I broke the combustor dataset into
  // three pieces and wrote them out separately.
  int i;
  vtkXMLStructuredGridReader* reader = vtkXMLStructuredGridReader::New();

  // vtkMultiBlockDataSet respresents multi-block datasets. See
  // the class documentation for more information.
  vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::New();

  for (i=0; i<3; i++)
  {
    // Here we load the three separate files (each containing
    // a structured grid dataset)
    std::ostringstream fname;
    fname << "Data/multicomb_" << i << ".vts" << ends;
    char* cfname =
      vtkTestUtilities::ExpandDataFileName(argc, argv, fname.str().c_str());
    reader->SetFileName(cfname);
    // We have to update since we are working without a VTK pipeline.
    // This will read the file and the output of the reader will be
    // a valid structured grid data.
    reader->Update();
    delete[] cfname;

    // We create a copy to avoid adding the same data three
    // times (the output object of the reader does not change
    // when the filename changes)
    vtkStructuredGrid* sg = vtkStructuredGrid::New();
    sg->ShallowCopy(reader->GetOutput());

    // Add the structured grid to the multi-block dataset
    mb->SetBlock(i, sg);
    sg->Delete();
  }
  reader->Delete();

  // Multi-block can be processed with regular VTK filters in two ways:
  // 1. Pass through a multi-block aware consumer. Since a multi-block
  //    aware mapper is not yet available, vtkCompositeDataGeometryFilter
  //    can be used
  // 2. Assign the composite executive (vtkCompositeDataPipeline) to
  //    all "simple" (that work only on simple, non-composite datasets) filters

  // outline
  vtkStructuredGridOutlineFilter* of = vtkStructuredGridOutlineFilter::New();
  of->SetInputData(mb);

  // geometry filter
  // This filter is multi-block aware and will request blocks from the
  // input. These blocks will be processed by simple processes as if they
  // are the whole dataset
  vtkCompositeDataGeometryFilter* geom1 =
    vtkCompositeDataGeometryFilter::New();
  geom1->SetInputConnection(0, of->GetOutputPort(0));

  // Rendering objects
  vtkPolyDataMapper* geoMapper = vtkPolyDataMapper::New();
  geoMapper->SetInputConnection(0, geom1->GetOutputPort(0));

  vtkActor* geoActor = vtkActor::New();
  geoActor->SetMapper(geoMapper);
  geoActor->GetProperty()->SetColor(0, 0, 0);
  ren->AddActor(geoActor);

  // cell 2 point and contour
  vtkCellDataToPointData* c2p = vtkCellDataToPointData::New();
  c2p->SetInputData(mb);

  vtkContourFilter* contour = vtkContourFilter::New();
  contour->SetInputConnection(0, c2p->GetOutputPort(0));
  contour->SetValue(0, 0.45);

  // geometry filter
  vtkCompositeDataGeometryFilter* geom2 =
    vtkCompositeDataGeometryFilter::New();
  geom2->SetInputConnection(0, contour->GetOutputPort(0));

  // Rendering objects
  vtkPolyDataMapper* contMapper = vtkPolyDataMapper::New();
  contMapper->SetInputConnection(0, geom2->GetOutputPort(0));

  vtkActor* contActor = vtkActor::New();
  contActor->SetMapper(contMapper);
  contActor->GetProperty()->SetColor(1, 0, 0);
  ren->AddActor(contActor);

  ren->SetBackground(1,1,1);
  renWin->SetSize(300,300);
  iren->Start();

  // Cleanup
  vtkAlgorithm::SetDefaultExecutivePrototype(0);
  of->Delete();
  geom1->Delete();
  geoMapper->Delete();
  geoActor->Delete();
  c2p->Delete();
  contour->Delete();
  geom2->Delete();
  contMapper->Delete();
  contActor->Delete();
  ren->Delete();
  renWin->Delete();
  iren->Delete();
  mb->Delete();

  return 0;
}
