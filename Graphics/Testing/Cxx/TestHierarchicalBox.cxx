/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHierarchicalBox.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example demonstrates how hierarchical box (uniform rectilinear)
// AMR datasets can be processed using the new vtkHierarchicalBoxDataSet class. 
// Since the native pipeline support is not yet available, helper classes are 
// used outside the pipeline to process the dataset. 
// See the comments below for details.
// 
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
// -D <path> => path to the data; the data should be in <path>/Data/

#include "vtkActor.h"
#include "vtkAMRBox.h"
#include "vtkCellDataToPointData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataVisitor.h"
#include "vtkContourFilter.h"
#include "vtkDebugLeaks.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkImageData.h"
#include "vtkMultiBlockApplyFilterCommand.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkUniformGrid.h"
#include "vtkXMLImageDataReader.h"

int TestHierarchicalBox(int argc, char* argv[])
{
  // Disable for testing
  vtkDebugLeaks::PromptUserOff();

  // Standard rendering classes
  vtkRenderer *ren = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // Since there is no AMR reader avaible yet, we will load a
  // collection of VTK files and create our own vtkHierarchicalBoxDataSet.
  // To create the files, I loaded a Chombo file with an experimental
  // Chombo reader and wrote the datasets separately.
  int i;
  vtkXMLImageDataReader* reader = vtkXMLImageDataReader::New();
  // vtkHierarchicalBoxDataSet represents hierarchical box 
  // (uniform rectilinear) AMR datasets, See the class documentation 
  // for more information.
  vtkHierarchicalBoxDataSet* hb = vtkHierarchicalBoxDataSet::New();

  for (i=0; i<16; i++)
    {
    // Here we load the 16 separate files (each containing
    // an image dataset -uniform rectilinear grid-)
    char* fstr = new char [strlen("Data/chombo3d/chombo3d_") + 
                          strlen(".vti") + 10];
    sprintf(fstr,"Data/chombo3d/chombo3d_%i.vti",i);
    char* cfname = 
      vtkTestUtilities::ExpandDataFileName(argc, argv, fstr);
    reader->SetFileName(cfname);
    // We have to update since we are working without a VTK pipeline.
    // This will read the file and the output of the reader will be
    // a valid image data.
    reader->Update();
    delete[] fstr;
    delete[] cfname;
    
    // We now create a vtkUniformGrid. This is essentially a simple
    // vtkImageData (not a sub-class though) with blanking. Since
    // VTK readers do not know vtkUniformGrid, we simply create our
    // own by copying from the image data.
    vtkUniformGrid* ug = vtkUniformGrid::New();
    ug->ShallowCopy(reader->GetOutput());

    // Each sub-dataset in a vtkHierarchicalBoxDataSet has an associated
    // vtkAMRBox. This is similar to extent but is stored externally
    // since it is possible to have sub-dataset nodes with NULL
    // vtkUniformGrid pointers.
    vtkAMRBox box;

    // This is a hack (do not do this at home). Normally, the
    // region (box) information should be available in the file.
    // In this case, since there is no such information available,
    // we obtain it by looking at each image data's extent.
    // -- begin hack
    int extent[6];
    double spacing[3];
    double origin[3];
    ug->GetExtent(extent);
    ug->GetSpacing(spacing);
    ug->GetOrigin(origin);

    for (int j=0; j<3; j++)
      {
      box.LoCorner[j] = 
        static_cast<int>(origin[j]/spacing[j] + extent[2*j]);
      box.HiCorner[j] = 
        static_cast<int>(origin[j]/spacing[j] + extent[2*j+1] - 1);
      }
    
    // Similarly, the level of each sub-dataset is normally 
    // available in the file. Since this is not the case, I
    // hard coded this into the example program.
    // Level 0 = { 0 }, Level 1 = { 1 }, 
    // Level 2 = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }
    int level;
    int dsindex;
    if (i == 0)
      {
      level = 0;
      dsindex = 0;
      }
    else if (i == 1)
      {
      level = 1;
      dsindex = 0;
      }
    else
      {
      level = 2;
      dsindex = i-2;
      }
    // -- end hack

    // Given the level, index and box, add the sub-dataset to
    // hierarchical dataset.
    hb->SetDataSet(level, dsindex, box, ug);

    ug->Delete();
    }
  reader->Delete();
  
  // I hard coded the refinement ratios. These should normally
  // be available in the file.
  hb->SetRefinementRatio(0, 2);
  hb->SetRefinementRatio(1, 2);

  // This call generates visibility (blanking) arrays that mask
  // regions of lower level datasets that overlap with regions
  // of higher level datasets (it is assumed that, when available,
  // higher level information should always be used instead of
  // lower level information)
  hb->GenerateVisibilityArrays();

  // Here is how a multi-block dataset is processed currently:
  // 1. Create a command to applied to each sub-dataset in the AMR
  //    dataset. Usually this is vtkMultiBlockApplyFilterCommand. This
  //    command applies a filter to each sub-dataset and collects the
  //    outputs in a multi-block dataset.
  // 2. Create a visitor that will iterate over the sub-datasets and
  //    apply the command to each.
  // 3. Get the output from the command.
  
  // First, we pass the AMR dataset through a celldatatopointdata filter
  // since the dataset has cell data only (contour needs point data).

  // Create the command
  vtkMultiBlockApplyFilterCommand* comm1 = 
    vtkMultiBlockApplyFilterCommand::New();

  // Create and assign the filter.
  vtkCellDataToPointData* c2p = vtkCellDataToPointData::New();
  comm1->SetFilter(c2p);
  c2p->Delete();

  // Ask the dataset to create an appropriate visitor for us.
  vtkCompositeDataVisitor* visitor1 = hb->NewVisitor();
  // Tell the visitor to use the command we create
  visitor1->SetCommand(comm1);

  // Apply the command to each sub-dataset.
  visitor1->Execute();

  // Next we apply an iso-surface filter to the resulting multi-block
  // dataset.

  // Create the command
  vtkMultiBlockApplyFilterCommand* comm2 = 
    vtkMultiBlockApplyFilterCommand::New();

  // Create and assign the filter.
  vtkContourFilter* contour = vtkContourFilter::New();
  // Note that we are setting the contour values directly
  // on the filter. There is no way of doing this through
  // the command or visitor.
  contour->SetValue(0, -0.013);
  contour->SelectInputScalars("phi");
  comm2->SetFilter(contour);
  contour->Delete();

  // Ask the multi-block datasets to create an appropriate visitor
  // for us.
  vtkCompositeDataVisitor* visitor2 = comm1->GetOutput()->NewVisitor();
  // Tell the visitor to use the command we create
  visitor2->SetCommand(comm2);

  // Apply the command to each sub-dataset. If any of the sub-datasets
  // are composite datasets, the visitor will recursively process those
  // and their sub-datasets.
  visitor2->Execute();

  // After the execution, the output should contain all the
  // iso-surfaces (one polydata for each sub-dataset)
  vtkMultiBlockDataSet* output = comm2->GetOutput();

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
  // vtkHBoxAMRSomethingReader* reader = vtkHBoxAMRSomethingReader::New()
  //
  // vtkHierarchicalBoxCellDataToPointData* c2p = 
  //   vtkHierarchicalBoxCellDataToPointData::New();
  // c2p->SetInput(reader->GetOutput());
  //
  // vtkContourFilter* contour = vtkContourFilter::New();
  // contour->SetInput(c2p->GetOutput());
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
  comm1->Delete();
  comm2->Delete();
  visitor1->Delete();
  visitor2->Delete();
  hb->Delete();

  return !retVal;
}
