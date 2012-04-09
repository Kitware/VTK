/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pipe1.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCallbackCommand.h"
#include "vtkImageData.h"
#include "vtkOutputPort.h"
#include "vtkRTAnalyticSource.h"

#include "PipelineParallelism.h"

static float XFreq = 60;

// Increments XFreq of the synthetic source
static void IncrementXFreq(vtkObject *vtkNotUsed( caller ),
                           unsigned long vtkNotUsed(eventId),
                           void *sr, void *)
{
  vtkRTAnalyticSource* source1 = reinterpret_cast<vtkRTAnalyticSource*>(sr);
  XFreq = XFreq + 10;
  source1->SetXFreq(XFreq);
}


// Pipe 1 for PipelineParallelism.
// See PipelineParallelism.cxx for more information.
void pipe1(vtkMultiProcessController* vtkNotUsed(controller),
           void* vtkNotUsed(arg))
{
  double extent = 20;
  int iextent = static_cast<int>(extent);

  // Synthetic image source.
  vtkRTAnalyticSource* source1 = vtkRTAnalyticSource::New();
  source1->SetWholeExtent (-1*iextent, iextent, -1*iextent, iextent,
                           -1*iextent, iextent );
  source1->SetCenter(0, 0, 0);
  source1->SetStandardDeviation( 0.5 );
  source1->SetMaximum( 255.0 );
  source1->SetXFreq( XFreq );
  source1->SetXMag( 10 );
  source1->SetYFreq( 30 );
  source1->SetYMag( 18 );
  source1->SetZFreq( 40 );
  source1->SetZMag( 5 );
  source1->GetOutput()->SetSpacing(2.0/extent,2.0/extent,2.0/extent);

  // Output port
  vtkOutputPort* op = vtkOutputPort::New();
  op->SetInputConnection(source1->GetOutputPort());
  op->SetTag(11);

  // Called every time data is requested from the output port
  vtkCallbackCommand *cbc = vtkCallbackCommand::New();
  cbc->SetCallback(IncrementXFreq);
  cbc->SetClientData((void *)source1);
  op->AddObserver(vtkCommand::EndEvent,cbc);
  cbc->Delete();

  // Process requests
  op->WaitForUpdate();

  // Cleanup
  op->Delete();
  source1->Delete();

}


