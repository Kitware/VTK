/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperOctreeDual.cxx

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
#include <cassert>
#include "vtkLookupTable.h"
#include "vtkPolyData.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkHyperOctreeDualGridContourFilter.h"
#include "vtkContourFilter.h"
#include "vtkProperty.h"
#include "vtkDataSetMapper.h"
#include "vtkPolyDataMapper.h"
#include "vtkTimerLog.h"
#include "vtkHyperOctreeFractalSource.h"
#include "vtkSphere.h"
#include "vtkCamera.h"

int TestHyperOctreeDual(int argc, char* argv[])
{
  // Standard rendering classes
  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  vtkTimerLog *timer=vtkTimerLog::New();

  // 3D
  vtkHyperOctreeFractalSource* source3d = vtkHyperOctreeFractalSource::New();
  source3d->SetMaximumNumberOfIterations(17);
  source3d->SetMaximumLevel(7);
  source3d->SetMinimumLevel(3);

  cout<<"update source3d..."<<endl;
  timer->StartTimer();
  source3d->Update(); // Update now, make things easier with a debugger
  timer->StopTimer();
  cout<<"source updated3d"<<endl;
  cout<<"source3d time="<<timer->GetElapsedTime()<<" s"<<endl;

  vtkHyperOctreeDualGridContourFilter *contour3d;
  contour3d = vtkHyperOctreeDualGridContourFilter::New();
  contour3d->SetNumberOfContours(2);
  contour3d->SetValue(0,4.5);
  contour3d->SetValue(1,10.5);

  contour3d->SetInputConnection(0,source3d->GetOutputPort(0));
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
  mapper3d->SetScalarRange(0, 17);

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
  vtkHyperOctreeFractalSource* source2d = vtkHyperOctreeFractalSource::New();
  source2d->SetDimension(2);
  source2d->SetMaximumNumberOfIterations(17);
  source2d->SetMaximumLevel(7);
  source2d->SetMinimumLevel(4);

  cout<<"update source2d..."<<endl;
  timer->StartTimer();
  source2d->Update(); // Update now, make things easier with a debugger
  timer->StopTimer();
  cout<<"source updated2d"<<endl;
  cout<<"source2d time="<<timer->GetElapsedTime()<<" s"<<endl;


  // This creates a blue to red lut.
  vtkLookupTable *lut2d = vtkLookupTable::New();
  lut2d->SetHueRange (0.667, 0.0);

  vtkDataSetMapper *mapper2d = vtkDataSetMapper::New();
  mapper2d->SetInputConnection(0,source2d->GetOutputPort(0));
  mapper2d->SetLookupTable(lut2d);
  mapper2d->SetScalarRange(0, 17);

  vtkActor *actor2d = vtkActor::New();
  actor2d->SetPosition(2.5,0,0);
  actor2d->SetOrientation(180,0,0);
  actor2d->SetMapper(mapper2d);
  actor2d->GetProperty()->SetRepresentationToWireframe();
  actor2d->GetProperty()->SetAmbient(1.0);
  actor2d->GetProperty()->SetDiffuse(0.0);
  renderer->AddActor(actor2d);

#ifdef WRITE_RESULT
  // Save the result of the filter in a file
  vtkXMLPolyDataWriter *writer2d=vtkXMLPolyDataWriter::New();
  writer2d->SetInputConnection(0,source2d->GetOutputPort(0));
  writer2d->SetFileName("dual2d.vtp");
  writer2d->SetDataModeToAscii();
  writer2d->Write();
  writer2d->Delete();
#endif // #ifdef WRITE_RESULT

  vtkContourFilter *contourDS=vtkContourFilter::New();
  contourDS->SetNumberOfContours(2);
  contourDS->SetValue(0,4.5);
  contourDS->SetValue(1,10.5);

  contourDS->SetInputConnection(0,source3d->GetOutputPort(0));
  cout<<"update contour data set..."<<endl;
  timer->StartTimer();
  contourDS->Update(); // Update now, make things easier with a debugger
  timer->StopTimer();
  cout<<"contour data set updated"<<endl;
  cout<<"contour data set time="<<timer->GetElapsedTime()<<" s"<<endl;

  // This creates a blue to red lut.
  vtkLookupTable *lutDS = vtkLookupTable::New();
  lutDS->SetHueRange (0.667, 0.0);

  vtkPolyDataMapper *mapperDS = vtkPolyDataMapper::New();
  mapperDS->SetInputConnection(0,contourDS->GetOutputPort(0));
  mapperDS->SetLookupTable(lutDS);
  mapperDS->SetScalarRange(0, 17);

  vtkActor *actorDS = vtkActor::New();
  actorDS->SetPosition(2.5,2.5,0);
  actorDS->SetMapper(mapperDS);
  renderer->AddActor(actorDS);

#ifdef WRITE_RESULT
  // Save the result of the filter in a file
  vtkXMLPolyDataWriter *writerDS=vtkXMLPolyDataWriter::New();
  writerDS->SetInputConnection(0,contourDS->GetOutputPort(0));
  writerDS->SetFileName("contourDS.vtp");
  writerDS->SetDataModeToAscii();
  writerDS->Write();
  writerDS->Delete();
#endif // #ifdef WRITE_RESULT

  // Standard testing code.
  renderer->SetBackground(0.5,0.5,0.5);
  renWin->SetSize(300,300);
  vtkCamera *cam=renderer->GetActiveCamera();
  renderer->ResetCamera();
  cam->Azimuth(180);
  cam->Zoom(1.35);
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  // Cleanup
  renderer->Delete();
  renWin->Delete();
  iren->Delete();

  contourDS->Delete();
  lutDS->Delete();
  mapperDS->Delete();
  actorDS->Delete();

  source3d->Delete();
  contour3d->Delete();
  mapper3d->Delete();
  actor3d->Delete();
  lut3d->Delete();

  source2d->Delete();
  mapper2d->Delete();
  actor2d->Delete();
  lut2d->Delete();

  timer->Delete();

  return !retVal;
}
