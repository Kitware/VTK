/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperOctreeContourFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example demonstrates how to use a vtkHyperOctreeSampleFunction and
// apply a vtkHyperOctreeCutter filter on it.
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
#include "vtkXMLPolyDataWriter.h"
#include "vtkHyperOctreeContourFilter.h"

#include "vtkPolyDataMapper.h"
#include "vtkTimerLog.h"
#include "vtkHyperOctreeSampleFunction.h"
#include "vtkSphere.h"
#include "vtkCamera.h"

int TestHyperOctreeContourFilter(int argc, char* argv[])
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
  
  vtkHyperOctreeContourFilter *contour3d=vtkHyperOctreeContourFilter::New();
  contour3d->SetNumberOfContours(3);
  contour3d->SetValue(0,0.5);
  contour3d->SetValue(1,4.0);
  contour3d->SetValue(2,8.0);
  
  contour3d->SetInputConnection(0,source3d->GetOutputPort(0));
  source3d->Delete();
  cout<<"update contour3d..."<<endl;
  timer->StartTimer();
  contour3d->Update(); // Update now, make things easier with a debugger
  timer->StopTimer();
  cout<<"contour3d updated"<<endl;
  cout<<"contour3d time="<<timer->GetElapsedTime()<<" s"<<endl;
  
  
  // This creates a blue to red lut.
  vtkLookupTable *lut3d = vtkLookupTable::New(); 
  lut3d->SetHueRange (0.667, 0.0);

  vtkPolyDataMapper *mapper3d = vtkPolyDataMapper::New();
  mapper3d->SetInputConnection(0, contour3d->GetOutputPort(0) );
  contour3d->Delete();
    
  mapper3d->SetLookupTable(lut3d);
  
  if(contour3d->GetOutput()->GetCellData()!=0)
    {
    if(contour3d->GetOutput()->GetCellData()->GetScalars()!=0)
      {
      mapper3d->SetScalarRange( contour3d->GetOutput()->GetCellData()->
                                GetScalars()->GetRange());
      }
    }
  
  vtkActor *actor3d = vtkActor::New();
  actor3d->SetMapper(mapper3d);
  renderer->AddActor(actor3d);
  
#ifdef WRITE_RESULT
  // Save the result of the filter in a file
  vtkXMLPolyDataWriter *writer3d=vtkXMLPolyDataWriter::New();
  writer3d->SetInputConnection(0,contour3d->GetOutputPort(0));
  writer3d->SetFileName("contour3d.vtp");
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
  
  
  vtkHyperOctreeContourFilter *contour2d=vtkHyperOctreeContourFilter::New();
  contour2d->SetNumberOfContours(3);
  contour2d->SetValue(0,0.5);
  contour2d->SetValue(1,4.0);
  contour2d->SetValue(2,8.0);
  
  contour2d->SetInputConnection(0,source2d->GetOutputPort(0));
  source2d->Delete();
  cout<<"update contour2d..."<<endl;
  timer->StartTimer();
  contour2d->Update(); // Update now, make things easier with a debugger
  timer->StopTimer();
  cout<<"contour2d updated"<<endl;
  cout<<"contour2d time="<<timer->GetElapsedTime()<<" s"<<endl;
  
  // This creates a blue to red lut.
  vtkLookupTable *lut2d = vtkLookupTable::New(); 
  lut2d->SetHueRange (0.667, 0.0);

  vtkPolyDataMapper *mapper2d = vtkPolyDataMapper::New();
  mapper2d->SetInputConnection(0,contour2d->GetOutputPort(0));
  mapper2d->SetLookupTable(lut2d);
  mapper2d->SetScalarModeToUseCellData();

  if(contour2d->GetOutput()->GetCellData()!=0)
    {
    if(contour2d->GetOutput()->GetCellData()->GetScalars()!=0)
      {
      mapper2d->SetScalarRange( contour2d->GetOutput()->GetCellData()->
                              GetScalars()->GetRange());
      }
    }
  
  vtkActor *actor2d = vtkActor::New();
  actor2d->SetPosition(5,0,0);
  actor2d->SetMapper(mapper2d);
  renderer->AddActor(actor2d);
  
#ifdef WRITE_RESULT
  // Save the result of the filter in a file
  vtkXMLPolyDataWriter *writer2d=vtkXMLPolyDataWriter::New();
  writer2d->SetInputConnection(0,contour2d->GetOutputPort(0));
  writer2d->SetFileName("contour2d.vtp");
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
  
  
  vtkHyperOctreeContourFilter *contour1d=vtkHyperOctreeContourFilter::New();
  contour1d->SetNumberOfContours(3);
  contour1d->SetValue(0,0.5);
  contour1d->SetValue(1,4.0);
  contour1d->SetValue(2,8.0);
  
  contour1d->SetInputConnection(0,source1d->GetOutputPort(0));
  source1d->Delete();
  cout<<"update contour1d..."<<endl;
  timer->StartTimer();
  contour1d->Update(); // Update now, make things easier with a debugger
  timer->StopTimer();
  cout<<"contour1d updated"<<endl;
  cout<<"contour1d time="<<timer->GetElapsedTime()<<" s"<<endl;
  
  // This creates a blue to red lut.
  vtkLookupTable *lut1d = vtkLookupTable::New(); 
  lut1d->SetHueRange (0.667, 0.0);

  vtkPolyDataMapper *mapper1d = vtkPolyDataMapper::New();
  mapper1d->SetInputConnection(0,contour1d->GetOutputPort(0));
  mapper1d->SetLookupTable(lut1d);
  
  if(contour1d->GetOutput()->GetCellData()!=0)
    {
    if(contour1d->GetOutput()->GetCellData()->GetScalars()!=0)
      {
      mapper1d->SetScalarRange( contour1d->GetOutput()->GetCellData()->
                              GetScalars()->GetRange());
      }
    }
  
  vtkActor *actor1d = vtkActor::New();
  actor1d->SetPosition(10,0,0);
  actor1d->SetMapper(mapper1d);
  renderer->AddActor(actor1d);
  
#ifdef WRITE_RESULT
  // Save the result of the filter in a file
  vtkXMLPolyDataWriter *writer1d=vtkXMLPolyDataWriter::New();
  writer1d->SetInputConnection(0,contour1d->GetOutputPort(0));
  writer1d->SetFileName("contour1d.vtp");
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
  lut3d->Delete();
  
  mapper2d->Delete();
  actor2d->Delete();
  lut2d->Delete();
  contour2d->Delete();

  mapper1d->Delete();
  actor1d->Delete();
  lut1d->Delete();
  contour1d->Delete();

  timer->Delete();
  
  return !retVal;
}
