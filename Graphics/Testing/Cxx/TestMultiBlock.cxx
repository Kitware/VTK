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
// This example demonstrates how multi-block datasets can be processed
// using the new vtkMultiBlockDataSet class. Since the native pipeline
// support is not yet available, helper classes are used outside the
// pipeline to process the dataset. See the comments below for details.
// 
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
// -D <path> => path to the data; the data should be in <path>/Data/

#include "vtkActor.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataVisitor.h"
#include "vtkContourFilter.h"
#include "vtkDebugLeaks.h"
#include "vtkMultiBlockApplyFilterCommand.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStructuredGrid.h"
#include "vtkXMLStructuredGridReader.h"

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
    ostrstream fname;
    fname << "Data/multicomb_" << i << ".vts" << ends;
    char* fstr = fname.str();
    char* cfname = 
      vtkTestUtilities::ExpandDataFileName(argc, argv, fstr);
    reader->SetFileName(cfname);
    // We have to update since we are working without a VTK pipeline.
    // This will read the file and the output of the reader will be
    // a valid structured grid data.
    reader->Update();
    delete[] fstr;
    delete[] cfname;

    // We create a copy to avoid adding the same data three
    // times (the output object of the reader does not change
    // when the filename changes)
    vtkStructuredGrid* sg = vtkStructuredGrid::New();
    sg->ShallowCopy(reader->GetOutput());

    // Add the structured grid to the multi-block dataset
    mb->AddDataSet(sg);
    sg->Delete();
    }
  reader->Delete();

  // Here is how a multi-block dataset is processed currently:
  // 1. Create a command to be applied to each sub-dataset in the multi-block
  //    dataset. Usually this is vtkMultiBlockApplyFilterCommand. This
  //    command applies a filter to each sub-dataset and collects the
  //    outputs in an another multi-block dataset.
  // 2. Create a visitor that will iterate over the sub-datasets and
  //    apply the command to each.
  // 3. Get the output from the command.

  // Create the command
  vtkMultiBlockApplyFilterCommand* comm = 
    vtkMultiBlockApplyFilterCommand::New();

  // Create and assign the filter.
  vtkContourFilter* filter = vtkContourFilter::New();
  // Note that we are setting the contour values directly
  // on the filter. There is no way of doing this through
  // the command or visitor.
  filter->SetValue(0, 0.45);
  comm->SetFilter(filter);
  filter->Delete();

  // Ask the multi-block dataset to create an appropriate visitor
  // for us.
  vtkCompositeDataVisitor* visitor = mb->NewVisitor();
  // Tell the visitor to use the command we create
  visitor->SetCommand(comm);

  // Apply the command to each sub-dataset. If any of the sub-datasets
  // are composite datasets, the visitor will recursively process those
  // and their sub-datasets.
  visitor->Execute();

  // After the execution, the output should contain all the
  // iso-surfaces (one polydata for each sub-dataset)
  vtkMultiBlockDataSet* output = comm->GetOutput();

  // We now create a mapper/actor pair for each iso-surface.

  // Ask the output multi-block dataset to create an appropriate
  // iterator. This is a forward iterator.
  vtkCompositeDataIterator* iter = output->NewIterator();
  iter->GoToFirstItem();
  while (!iter->IsDoneWithTraversal())
    {
    // For each polydata, create a mapper/actor pair
    vtkPolyData* pd = vtkPolyData::SafeDownCast(iter->GetCurrentDataObject());
    if (pd)
      {
      vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
      mapper->SetInput(pd);
      vtkActor* actor = vtkActor::New();
      actor->SetMapper(mapper);
      mapper->Delete();
      ren->AddActor(actor);
      actor->Delete();
      }
    iter->GoToNextItem();
    }
  iter->Delete();

  // In the future (once the pipeline changes are finished), it
  // will be possible to do the following:
  // 
  // vtkMultiBlockSomethingReader* reader = vtkMultiBlockSomethingReader::New()
  //
  // vtkContourFilter* contour = vtkContourFilter::New();
  // contour->SetInput(reader->GetOutput());
  //
  // /* This might or might not be necessary */
  // vtkMultiBlockPolyDataGeometryFilter* geom =  
  //      vtkMultiBlockPolyDataGeometryFilter::New();
  // geom->SetInput(contour->GetOutput());
  //
  // vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
  // mapper->SetInput(geom->GetOutput());

  // Standard testing code.
  ren->SetBackground(1,1,1);
  renWin->SetSize(300,300);
  renWin->Render();
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  // Cleanup
  ren->Delete();
  renWin->Delete();
  iren->Delete();
  comm->Delete();
  visitor->Delete();
  mb->Delete();

  return !retVal;
}
