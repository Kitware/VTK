/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperOctreeToUniformGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example demonstrates how to use a vtkHyperOctreeSampleFunction and
// apply a vtkHyperOctreeToUniformGrid on it.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
// -D <path> => path to the data; the data should be in <path>/Data/

// If WRITE_RESULT is defined, the result of the surface filter is saved.
#define WRITE_RESULT

#include "vtkActor.h"
#include "vtkDebugLeaks.h"
#include "vtkCellData.h"
#include "vtkProperty.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include <cassert>
#include "vtkLookupTable.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkHyperOctreeToUniformGridFilter.h"
#include "vtkDataSetMapper.h"
#include "vtkImageData.h"
#include "vtkImageWriter.h"
#include "vtkMetaImageWriter.h"
#include "vtkHyperOctreeSampleFunction.h"
#include "vtkSphere.h"
#include "vtkTimerLog.h"
#include "vtkCamera.h"
#include "vtkGeometryFilter.h"
#include "vtkPolyDataMapper.h"

int TestHyperOctreeToUniformGrid(int argc, char* argv[])
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
  source3d->SetLevels(5);
  source3d->SetMinLevels(0);

  cout<<"update source3d..."<<endl;
  timer->StartTimer();
  source3d->Update(); // Update now, make things easier with a debugger
  timer->StopTimer();
  cout<<"source3d updated"<<endl;
  cout<<"source3d time="<<timer->GetElapsedTime()<<" s"<<endl;

  vtkHyperOctreeToUniformGridFilter *flat3d=vtkHyperOctreeToUniformGridFilter::New();

  flat3d->SetInputConnection(0,source3d->GetOutputPort(0));
  source3d->Delete();

  cout<<"update flat3d..."<<endl;
  timer->StartTimer();
  flat3d->Update(); //So that we can call GetRange() on the scalars
  timer->StopTimer();
  cout<<"flat3d updated"<<endl;
  cout<<"flat3d time="<<timer->GetElapsedTime()<<" s"<<endl;

  // This creates a blue to red lut.
  vtkLookupTable *lut3d = vtkLookupTable::New();
  lut3d->SetHueRange (0.667, 0.0);

  vtkDataSetMapper *mapper3d = vtkDataSetMapper::New();
  mapper3d->SetInputConnection(0, flat3d->GetOutputPort(0) );
  mapper3d->SetLookupTable(lut3d);

  if(flat3d->GetOutput()->GetCellData()!=0)
  {
    if(flat3d->GetOutput()->GetCellData()->GetScalars()!=0)
    {
      mapper3d->SetScalarRange( flat3d->GetOutput()->GetCellData()->
                                GetScalars()->GetRange());
    }
  }

  vtkActor *actor3d = vtkActor::New();
  actor3d->SetMapper(mapper3d);
  renderer->AddActor(actor3d);

#ifdef WRITE_RESULT
  // Save the result of the filter in a file
  vtkXMLImageDataWriter *writer3d=vtkXMLImageDataWriter::New();
  writer3d->SetInputConnection(0,flat3d->GetOutputPort(0));
  writer3d->SetFileName("image3d.vti");
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
  source2d->SetLevels(10);
  source2d->SetMinLevels(0);

  cout<<"update source2d..."<<endl;
  timer->StartTimer();
  source2d->Update(); // Update now, make things easier with a debugger
  timer->StopTimer();
  cout<<"source2d updated"<<endl;
  cout<<"source2d time="<<timer->GetElapsedTime()<<" s"<<endl;

  vtkHyperOctreeToUniformGridFilter *flat2d=vtkHyperOctreeToUniformGridFilter::New();

  flat2d->SetInputConnection(0,source2d->GetOutputPort(0));
  source2d->Delete();

  cout<<"update flat2d..."<<endl;
  timer->StartTimer();
  flat2d->Update(); //So that we can call GetRange() on the scalars
  timer->StopTimer();
  cout<<"flat2d updated"<<endl;
  cout<<"flat3d time="<<timer->GetElapsedTime()<<" s"<<endl;

  // This creates a blue to red lut.
  vtkLookupTable *lut2d = vtkLookupTable::New();
  lut2d->SetHueRange (0.667, 0.0);

  vtkDataSetMapper *mapper2d = vtkDataSetMapper::New();
  mapper2d->SetInputConnection(0, flat2d->GetOutputPort(0) );
  mapper2d->SetLookupTable(lut2d);

  if(flat2d->GetOutput()->GetCellData()!=0)
  {
    if(flat2d->GetOutput()->GetCellData()->GetScalars()!=0)
    {
      mapper2d->SetScalarRange( flat2d->GetOutput()->GetCellData()->
                                GetScalars()->GetRange());
    }
  }

  vtkActor *actor2d = vtkActor::New();
  actor2d->SetPosition(5,0,0);
  actor2d->SetMapper(mapper2d);
  renderer->AddActor(actor2d);

#ifdef WRITE_RESULT
  // Save the result of the filter in a file
  vtkXMLImageDataWriter *writer2d=vtkXMLImageDataWriter::New();
  writer2d->SetInputConnection(0,flat2d->GetOutputPort(0));
  writer2d->SetFileName("image2d.vti");
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
  source1d->SetLevels(10);
  source1d->SetMinLevels(0);

  cout<<"update source1d..."<<endl;
  timer->StartTimer();
  source1d->Update(); // Update now, make things easier with a debugger
  timer->StopTimer();
  cout<<"source1d updated"<<endl;
  cout<<"source1d time="<<timer->GetElapsedTime()<<" s"<<endl;

  vtkHyperOctreeToUniformGridFilter *flat1d=vtkHyperOctreeToUniformGridFilter::New();

  flat1d->SetInputConnection(0,source1d->GetOutputPort(0));
  source1d->Delete();

  cout<<"update flat1d..."<<endl;
  timer->StartTimer();
  flat1d->Update(); //So that we can call GetRange() on the scalars
  timer->StopTimer();
  cout<<"flat1d updated"<<endl;
  cout<<"flat3d time="<<timer->GetElapsedTime()<<" s"<<endl;

  // This creates a blue to red lut.
  vtkLookupTable *lut1d = vtkLookupTable::New();
  lut1d->SetHueRange (0.667, 0.0);

  // for 1D, we use a geometryfilter+polydatammaper instead of
  // a datasetmapper, because there is a bug in datasetmapper in
  // this case.
  vtkGeometryFilter *poly1d=vtkGeometryFilter::New();
  poly1d->SetInputConnection(0,flat1d->GetOutputPort(0));
  poly1d->Update(); // to call GetRange()

  vtkPolyDataMapper *mapper1d = vtkPolyDataMapper::New();
  mapper1d->SetInputConnection(0,poly1d->GetOutputPort(0));


  mapper1d->SetLookupTable(lut1d);

  if(flat1d->GetOutput()->GetCellData()!=0)
  {
    if(flat1d->GetOutput()->GetCellData()->GetScalars()!=0)
    {
      mapper1d->SetScalarRange( flat1d->GetOutput()->GetCellData()->
                                GetScalars()->GetRange());
    }
  }

  vtkActor *actor1d = vtkActor::New();
  actor1d->SetPosition(10,0,0);
  actor1d->SetMapper(mapper1d);
  renderer->AddActor(actor1d);

#ifdef WRITE_RESULT
  // Save the result of the filter in a file
  vtkXMLImageDataWriter *writer1d=vtkXMLImageDataWriter::New();
  writer1d->SetInputConnection(0,flat1d->GetOutputPort(0));
  writer1d->SetFileName("image1d.vti");
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
  flat3d->Delete();
  lut3d->Delete();

  mapper2d->Delete();
  actor2d->Delete();
  flat2d->Delete();
  lut2d->Delete();

  mapper1d->Delete();
  actor1d->Delete();
  flat1d->Delete();
  lut1d->Delete();
  poly1d->Delete();

  timer->Delete();

  return !retVal;
}
