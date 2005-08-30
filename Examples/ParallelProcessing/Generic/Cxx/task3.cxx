/*=========================================================================

  Program:   Visualization Toolkit
  Module:    task3.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "TaskParallelismWithPorts.h"

#include "vtkAppendPolyData.h"
#include "vtkImageData.h"
#include "vtkInputPort.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

// Task 3 for TaskParallelismWithPorts.
// See TaskParallelismWithPorts.cxx for more information.
void task3(double data)
{
  double extent = data;
  int iextent = static_cast<int>(data);  
  // The pipeline

  // Synthetic image source.
  vtkRTAnalyticSource* source1 = vtkRTAnalyticSource::New();
  source1->SetWholeExtent (-1*iextent, iextent, -1*iextent, iextent, 
                           -1*iextent, iextent );
  source1->SetCenter(0, 0, 0);
  source1->SetStandardDeviation( 0.5 );
  source1->SetMaximum( 255.0 );
  source1->SetXFreq( 60 );
  source1->SetXMag( 10 );
  source1->SetYFreq( 30 );
  source1->SetYMag( 18 );
  source1->SetZFreq( 40 );
  source1->SetZMag( 5 );
  source1->GetOutput()->SetSpacing(2.0/extent,2.0/extent,2.0/extent);

  // Iso-surfacing.
  vtkContourFilter* contour = vtkContourFilter::New();
  contour->SetInputConnection(source1->GetOutputPort());
  contour->SetNumberOfContours(1);
  contour->SetValue(0, 220);

  // Magnitude of the gradient vector.
  vtkImageGradientMagnitude* magn = vtkImageGradientMagnitude::New();
  magn->SetDimensionality(3);
  magn->SetInputConnection(source1->GetOutputPort());

  // Probe magnitude with iso-surface.
  vtkProbeFilter* probe = vtkProbeFilter::New();
  probe->SetInputConnection(contour->GetOutputPort());
  probe->SetSource(magn->GetOutput());
  probe->SpatialMatchOn();

  // Input port
  vtkInputPort* ip = vtkInputPort::New();
  ip->SetRemoteProcessId(1);
  ip->SetTag(11);

  // Append the local and remote data
  vtkAppendPolyData* append = vtkAppendPolyData::New();
  append->AddInput(ip->GetPolyDataOutput());
  append->AddInput(probe->GetPolyDataOutput());
  

  // Rendering objects.
  vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(append->GetOutputPort());
  mapper->SetScalarRange(50, 180);
  mapper->ImmediateModeRenderingOn();

  vtkActor* actor = vtkActor::New();
  actor->SetMapper(mapper);

  // Create the render objects
  vtkRenderWindow* renWin = vtkRenderWindow::New();
  renWin->SetSize( WINDOW_WIDTH, WINDOW_HEIGHT );

  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  vtkRenderer* ren = vtkRenderer::New();
  renWin->AddRenderer(ren);

  ren->AddActor(actor);

  iren->Initialize();
  iren->Start();

  // Tell the other process we are done
  ip->GetController()->TriggerRMI(1, 
                                  vtkMultiProcessController::BREAK_RMI_TAG); 

  // Cleanup
  iren->Delete();
  renWin->Delete();
  ip->Delete();
  append->Delete();
  source1->Delete();
  contour->Delete();
  magn->Delete();
  probe->Delete();
  actor->Delete();
  ren->Delete();
  mapper->Delete();

}


