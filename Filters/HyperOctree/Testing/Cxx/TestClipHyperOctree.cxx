/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestClipHyperOctree.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example demonstrates how to use a vtkHyperOctreeSampleFunction and
// apply a vtkClipHyperOctree filter on it.
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
#include <assert.h>
#include "vtkLookupTable.h"
#include "vtkPolyData.h"
#include "vtkXMLUnstructuredGridWriter.h"
#include "vtkGeometryFilter.h"
#include "vtkClipHyperOctree.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPlane.h"

#include "vtkPolyDataMapper.h"
#include "vtkTimerLog.h"
#include "vtkHyperOctreeSampleFunction.h"
#include "vtkSphere.h"
#include "vtkCamera.h"

int TestClipHyperOctree(int argc, char* argv[])
{
  // Standard rendering classes
  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(renderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  
  vtkTimerLog *timer=vtkTimerLog::New();
  
  // 3D
  vtkHyperOctreeSampleFunction *source3d=vtkHyperOctreeSampleFunction::New();
  vtkSphere *f3d=vtkSphere::New();
  f3d->SetRadius(1);
  f3d->SetCenter(1,1,0);
  source3d->SetImplicitFunction(f3d);
  source3d->SetThreshold(0.2);
  f3d->Delete();

  source3d->SetDimension(3);
  source3d->SetWidth(2);
  source3d->SetHeight(3);
  source3d->SetDepth(4);
  source3d->SetLevels(6); // 10
  source3d->SetMinLevels(0);
 
  cout<<"update source3d..."<<endl;
  timer->StartTimer();
  source3d->Update(); // Update now, make things easier with a debugger
  timer->StopTimer();
  cout<<"source updated3d"<<endl;
  cout<<"source3d time="<<timer->GetElapsedTime()<<" s"<<endl;
  
  vtkClipHyperOctree *clipper3d=vtkClipHyperOctree::New();
  vtkPlane *p3d=vtkPlane::New();
  p3d->SetOrigin(0.4,0.4,0.4);
  p3d->SetNormal(1,1,1);
  clipper3d->SetClipFunction(p3d);
  p3d->Delete();
  clipper3d->SetInputConnection(0,source3d->GetOutputPort(0));
  source3d->Delete();
  cout<<"update clipper3d..."<<endl;
  timer->StartTimer();
  clipper3d->Update(); // Update now, make things easier with a debugger
  timer->StopTimer();
  cout<<"clipper3d updated"<<endl;
  cout<<"clipper3d time="<<timer->GetElapsedTime()<<" s"<<endl;

  vtkGeometryFilter *surface3d=vtkGeometryFilter::New();
  surface3d->SetInputConnection(0,clipper3d->GetOutputPort(0));
  clipper3d->Delete();
  
  cout<<"update surface3d..."<<endl;
  surface3d->Update(); //So that we can call GetRange() on the scalars
  cout<<"surface3d updated"<<endl;
  
  assert(surface3d->GetOutput()!=0);
  
  // This creates a blue to red lut.
  vtkLookupTable *lut3d = vtkLookupTable::New(); 
  lut3d->SetHueRange (0.667, 0.0);

  vtkPolyDataMapper *mapper3d = vtkPolyDataMapper::New();
  mapper3d->SetInputConnection(0,surface3d->GetOutputPort(0));
  mapper3d->SetLookupTable(lut3d);
  
  if(surface3d->GetOutput()->GetCellData()!=0)
    {
    if(surface3d->GetOutput()->GetCellData()->GetScalars()!=0)
      {
      mapper3d->SetScalarRange( surface3d->GetOutput()->GetCellData()->
                              GetScalars()->GetRange());
      }
    }
  
  vtkActor *actor3d = vtkActor::New();
  actor3d->SetMapper(mapper3d);
  renderer->AddActor(actor3d);
  
#ifdef WRITE_RESULT
  // Save the result of the filter in a file
  vtkXMLUnstructuredGridWriter *writer3d=vtkXMLUnstructuredGridWriter::New();
  writer3d->SetInputConnection(0,clipper3d->GetOutputPort(0));
  writer3d->SetFileName("clip3d.vtu");
  writer3d->SetDataModeToAscii();
  writer3d->Write();
  writer3d->Delete();
#endif // #ifdef WRITE_RESULT

  // 2D
  vtkHyperOctreeSampleFunction *source2d=vtkHyperOctreeSampleFunction::New();
  vtkSphere *f2d=vtkSphere::New();
  f2d->SetRadius(1);
  f2d->SetCenter(1,1,0);
  source2d->SetImplicitFunction(f2d);
  source2d->SetThreshold(0.2);
  f2d->Delete();

  source2d->SetDimension(2);
  source2d->SetWidth(2);
  source2d->SetHeight(3);
  source2d->SetDepth(4);
  source2d->SetLevels(10); // 7
  source2d->SetMinLevels(0);
 
  cout<<"update source2d..."<<endl;
  timer->StartTimer();
  source2d->Update(); // Update now, make things easier with a debugger
  timer->StopTimer();
  cout<<"source updated2d"<<endl;
  cout<<"source2d time="<<timer->GetElapsedTime()<<" s"<<endl;
  
  vtkClipHyperOctree *clipper2d=vtkClipHyperOctree::New();
  vtkPlane *p2d=vtkPlane::New();
  p2d->SetOrigin(0.4,0.4,0.4);
  p2d->SetNormal(1,1,1);
  clipper2d->SetClipFunction(p2d);
  p2d->Delete();
  clipper2d->SetInputConnection(0,source2d->GetOutputPort(0));
  source2d->Delete();
  cout<<"update clipper2d..."<<endl;
  timer->StartTimer();
  clipper2d->Update(); // Update now, make things easier with a debugger
  timer->StopTimer();
  cout<<"clipper2d updated"<<endl;
  cout<<"clipper2d time="<<timer->GetElapsedTime()<<" s"<<endl;

  vtkGeometryFilter *surface2d=vtkGeometryFilter::New();
  surface2d->SetInputConnection(0,clipper2d->GetOutputPort(0));
  clipper2d->Delete();
  
  cout<<"update surface2d..."<<endl;
  surface2d->Update(); //So that we can call GetRange() on the scalars
  cout<<"surface2d updated"<<endl;
  
  assert(surface2d->GetOutput()!=0);
  
  // This creates a blue to red lut.
  vtkLookupTable *lut2d = vtkLookupTable::New(); 
  lut2d->SetHueRange (0.667, 0.0);

  vtkPolyDataMapper *mapper2d = vtkPolyDataMapper::New();
  mapper2d->SetInputConnection(0,surface2d->GetOutputPort(0));
  mapper2d->SetLookupTable(lut2d);
  
  if(surface2d->GetOutput()->GetCellData()!=0)
    {
    if(surface2d->GetOutput()->GetCellData()->GetScalars()!=0)
      {
      mapper2d->SetScalarRange( surface2d->GetOutput()->GetCellData()->
                              GetScalars()->GetRange());
      }
    }
  
  vtkActor *actor2d = vtkActor::New();
  actor2d->SetPosition(5,0,0);
  actor2d->SetMapper(mapper2d);
  renderer->AddActor(actor2d);
  
#ifdef WRITE_RESULT
  // Save the result of the filter in a file
  vtkXMLUnstructuredGridWriter *writer2d=vtkXMLUnstructuredGridWriter::New();
  writer2d->SetInputConnection(0,clipper2d->GetOutputPort(0));
  writer2d->SetFileName("clip2d.vtu");
  writer2d->SetDataModeToAscii();
  writer2d->Write();
  writer2d->Delete();
#endif // #ifdef WRITE_RESULT

  // 1D
  vtkHyperOctreeSampleFunction *source1d=vtkHyperOctreeSampleFunction::New();
  vtkSphere *f1d=vtkSphere::New();
  f1d->SetRadius(1);
  f1d->SetCenter(1,1,0);
  source1d->SetImplicitFunction(f1d);
  source1d->SetThreshold(0.2);
  f1d->Delete();

  source1d->SetDimension(1);
  source1d->SetWidth(2);
  source1d->SetHeight(3);
  source1d->SetDepth(4);
  source1d->SetLevels(10); // 7
  source1d->SetMinLevels(0);
 
  cout<<"update source1d..."<<endl;
  timer->StartTimer();
  source1d->Update(); // Update now, make things easier with a debugger
  timer->StopTimer();
  cout<<"source updated1d"<<endl;
  cout<<"source1d time="<<timer->GetElapsedTime()<<" s"<<endl;
  
  vtkClipHyperOctree *clipper1d=vtkClipHyperOctree::New();
  vtkPlane *p1d=vtkPlane::New();
  p1d->SetOrigin(0.4,0.4,0.4);
  p1d->SetNormal(1,1,1);
  clipper1d->SetClipFunction(p1d);
  p1d->Delete();
  clipper1d->SetInputConnection(0,source1d->GetOutputPort(0));
  source1d->Delete();
  cout<<"update clipper1d..."<<endl;
  timer->StartTimer();
  clipper1d->Update(); // Update now, make things easier with a debugger
  timer->StopTimer();
  cout<<"clipper1d updated"<<endl;
  cout<<"clipper1d time="<<timer->GetElapsedTime()<<" s"<<endl;

  vtkGeometryFilter *surface1d=vtkGeometryFilter::New();
  surface1d->SetInputConnection(0,clipper1d->GetOutputPort(0));
  clipper1d->Delete();
  
  cout<<"update surface1d..."<<endl;
  surface1d->Update(); //So that we can call GetRange() on the scalars
  cout<<"surface1d updated"<<endl;
  
  assert(surface1d->GetOutput()!=0);
  
  // This creates a blue to red lut.
  vtkLookupTable *lut1d = vtkLookupTable::New(); 
  lut1d->SetHueRange (0.667, 0.0);

  vtkPolyDataMapper *mapper1d = vtkPolyDataMapper::New();
  mapper1d->SetInputConnection(0,surface1d->GetOutputPort(0));
  mapper1d->SetLookupTable(lut1d);
  
  if(surface1d->GetOutput()->GetCellData()!=0)
    {
    if(surface1d->GetOutput()->GetCellData()->GetScalars()!=0)
      {
      mapper1d->SetScalarRange( surface1d->GetOutput()->GetCellData()->
                              GetScalars()->GetRange());
      }
    }
  
  vtkActor *actor1d = vtkActor::New();
  actor1d->SetPosition(10,0,0);
  actor1d->SetMapper(mapper1d);
  renderer->AddActor(actor1d);
  
#ifdef WRITE_RESULT
  // Save the result of the filter in a file
  vtkXMLUnstructuredGridWriter *writer1d=vtkXMLUnstructuredGridWriter::New();
  writer1d->SetInputConnection(0,clipper1d->GetOutputPort(0));
  writer1d->SetFileName("clip1d.vtu");
  writer1d->SetDataModeToAscii();
  writer1d->Write();
  writer1d->Delete();
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
  
  mapper3d->Delete();
  actor3d->Delete();
  surface3d->Delete();
  lut3d->Delete();
  
  mapper2d->Delete();
  actor2d->Delete();
  surface2d->Delete();
  lut2d->Delete();
  
  mapper1d->Delete();
  actor1d->Delete();
  surface1d->Delete();
  lut1d->Delete();
  
  timer->Delete();
  
  return !retVal;
}
