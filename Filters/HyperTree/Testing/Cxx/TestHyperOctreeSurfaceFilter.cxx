/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperOctreeSurfaceFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example demonstrates how to use a vtkHierarchicalDataSet of
// vtkHyperOctree created by some vtkHyperOctreeSampleFunction and apply a
// vtkHyperOctreeSurfaceFilter on it.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
// -D <path> => path to the data; the data should be in <path>/Data/

// If WRITE_RESULT is defined, the result of the surface filter is saved.
//#define WRITE_RESULT

#include "vtkActor.h"
#include "vtkCellData.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include <cassert>
#include "vtkLookupTable.h"
#include "vtkPolyData.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkHyperOctreeSurfaceFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkHyperOctreeSampleFunction.h"
#include "vtkSphere.h"
#include "vtkTimerLog.h"
#include "vtkCamera.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositePolyDataMapper.h"

int TestHyperOctreeSurfaceFilter(int argc, char* argv[])
{
  // Standard rendering classes
  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(renderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  vtkTimerLog *timer=vtkTimerLog::New();

  vtkMultiBlockDataSet *hds=vtkMultiBlockDataSet::New();
  hds->SetNumberOfBlocks(3);

  // 3D
  vtkHyperOctreeSampleFunction *source3d=vtkHyperOctreeSampleFunction::New();
  vtkSphere *f3d=vtkSphere::New();
  f3d->SetRadius(1);
  f3d->SetCenter(11,1,0);
  source3d->SetImplicitFunction(f3d);
  source3d->SetThreshold(0.2);
  f3d->Delete();

  source3d->SetDimension(3);
  source3d->SetWidth(2);
  source3d->SetHeight(3);
  source3d->SetDepth(4);
  source3d->SetLevels(7); // 10
  source3d->SetMinLevels(0);
  source3d->SetOrigin(10,0,0);

  cout<<"update source3d..."<<endl;
  timer->StartTimer();
  source3d->Update(); // Update now, make things easier with a debugger
  timer->StopTimer();
  cout<<"source3d updated"<<endl;
  cout<<"source3d time="<<timer->GetElapsedTime()<<" s"<<endl;

  hds->SetBlock(0,source3d->GetOutput());

  // 2D
  vtkHyperOctreeSampleFunction *source2d=vtkHyperOctreeSampleFunction::New();
  vtkSphere *f2d=vtkSphere::New();
  f2d->SetRadius(1);
  f2d->SetCenter(16,1,0);
  source2d->SetImplicitFunction(f2d);
  source2d->SetThreshold(0.2);
  f2d->Delete();

  source2d->SetDimension(2);
  source2d->SetWidth(2);
  source2d->SetHeight(3);
  source2d->SetDepth(4);
  source2d->SetLevels(10);
  source2d->SetMinLevels(0);
  source2d->SetOrigin(15,0,0);

  cout<<"update source2d..."<<endl;
  timer->StartTimer();
  source2d->Update(); // Update now, make things easier with a debugger
  timer->StopTimer();
  cout<<"source2d updated"<<endl;
  cout<<"source2d time="<<timer->GetElapsedTime()<<" s"<<endl;

  hds->SetBlock(1,source2d->GetOutput());
  source2d->Delete();

  // 1D
  vtkHyperOctreeSampleFunction *source1d=vtkHyperOctreeSampleFunction::New();
  vtkSphere *f1d=vtkSphere::New();
  f1d->SetRadius(1);
  f1d->SetCenter(21,1,0);
  source1d->SetImplicitFunction(f1d);
  source1d->SetThreshold(0.2);
  f1d->Delete();

  source1d->SetDimension(1);
  source1d->SetWidth(2);
  source1d->SetHeight(3);
  source1d->SetDepth(4);
  source1d->SetLevels(10);
  source1d->SetMinLevels(0);
  source1d->SetOrigin(20,0,0);

  cout<<"update source1d..."<<endl;
  timer->StartTimer();
  source1d->Update(); // Update now, make things easier with a debugger
  timer->StopTimer();
  cout<<"source1d updated"<<endl;
  cout<<"source1d time="<<timer->GetElapsedTime()<<" s"<<endl;

  hds->SetBlock(2,source1d->GetOutput());
  source1d->Delete();

  vtkHyperOctreeSurfaceFilter *surface=vtkHyperOctreeSurfaceFilter::New();
  vtkCompositeDataPipeline *exec = vtkCompositeDataPipeline::New();
  // Make sure we call SetExecutive right after the filter creation and
  // before the SetInput call.
  surface->SetExecutive(exec);
  surface->SetInputData(hds);
  hds->Delete();
  exec->Delete();

  cout<<"update surface..."<<endl;
  timer->StartTimer();
  surface->Update(); //So that we can call GetRange() on the scalars
  timer->StopTimer();
  cout<<"surface updated"<<endl;
  cout<<"surface time="<<timer->GetElapsedTime()<<" s"<<endl;


  // This creates a blue to red lut.
  vtkLookupTable *lut = vtkLookupTable::New();
  lut->SetHueRange (0.667, 0.0);

  vtkCompositePolyDataMapper *mapper=vtkCompositePolyDataMapper::New();
  mapper->SetInputConnection(0,surface->GetOutputPort(0));
  mapper->SetLookupTable(lut);

  if(source3d->GetOutput()->GetLeafData()!=0)
    {
    if(source3d->GetOutput()->GetLeafData()->GetScalars()!=0)
      {
      mapper->SetScalarRange( source3d->GetOutput()->GetLeafData()->
                              GetScalars()->GetRange());
      }
    }

  source3d->Delete();

  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

#ifdef WRITE_RESULT
  // Save the result of the filter in a file
  vtkXMLPolyDataWriter *writer3d=vtkXMLPolyDataWriter::New();
  writer3d->SetInputConnection(0,surface3d->GetOutputPort(0));
  writer3d->SetFileName("surface3d.vtp");
  writer3d->SetDataModeToAscii();
  writer3d->Write();
  writer3d->Delete();
#endif // #ifdef WRITE_RESULT

  // Standard testing code.
  renderer->SetBackground(0.5,0.5,0.5);
  renWin->SetSize(300,300);
  vtkCamera *cam=renderer->GetActiveCamera();
  renderer->ResetCamera();
  cam->Azimuth(180);

  renWin->Render();
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  // Cleanup
  renderer->Delete();
  renWin->Delete();
  iren->Delete();

  mapper->Delete();
  actor->Delete();
  surface->Delete();
  lut->Delete();

  timer->Delete();

  return !retVal;
}
