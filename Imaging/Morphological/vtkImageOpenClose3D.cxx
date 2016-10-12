/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageOpenClose3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageOpenClose3D.h"

#include "vtkGarbageCollector.h"
#include "vtkImageData.h"
#include "vtkImageDilateErode3D.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkExecutive.h"

#include <cmath>

vtkStandardNewMacro(vtkImageOpenClose3D);

//----------------------------------------------------------------------------
// functions to convert progress calls.
class vtkImageOpenClose3DProgress : public vtkCommand
{
public:
  // generic new method
  static vtkImageOpenClose3DProgress *New()
    { return new vtkImageOpenClose3DProgress; }

  // the execute
  void Execute(vtkObject *caller,
                       unsigned long event, void* vtkNotUsed(v)) VTK_OVERRIDE
  {
      vtkAlgorithm *alg = vtkAlgorithm::SafeDownCast(caller);
      if (event == vtkCommand::ProgressEvent && alg)
      {
        this->Self->UpdateProgress(this->Offset + 0.5 *
                                   alg->GetProgress());
      }
  }

  // some ivars that should be set
  vtkImageOpenClose3D *Self;
  double Offset;
};

//----------------------------------------------------------------------------
vtkImageOpenClose3D::vtkImageOpenClose3D()
{
  // create the filter chain
  this->Filter0 = vtkImageDilateErode3D::New();
  vtkImageOpenClose3DProgress *cb = vtkImageOpenClose3DProgress::New();
  cb->Self = this;
  cb->Offset = 0;
  this->Filter0->AddObserver(vtkCommand::ProgressEvent, cb);
  cb->Delete();

  this->Filter1 = vtkImageDilateErode3D::New();
  cb = vtkImageOpenClose3DProgress::New();
  cb->Self = this;
  cb->Offset = 0.5;
  this->Filter1->AddObserver(vtkCommand::ProgressEvent, cb);
  cb->Delete();
  this->SetOpenValue(0.0);
  this->SetCloseValue(255.0);

  // connect up the internal pipeline
  this->Filter1->SetInputConnection(this->Filter0->GetOutputPort());
}

//----------------------------------------------------------------------------
// Destructor: Delete the sub filters.
vtkImageOpenClose3D::~vtkImageOpenClose3D()
{
  if (this->Filter0)
  {
    this->Filter0->Delete();
  }

  if (this->Filter1)
  {
    this->Filter1->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkImageOpenClose3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Filter0: \n";
  this->Filter0->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Filter1: \n";
  this->Filter1->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
// Turn debugging output on. (in sub filters also)
void vtkImageOpenClose3D::DebugOn()
{
  this->vtkObject::DebugOn();
  if (this->Filter0)
  {
    this->Filter0->DebugOn();
  }
  if (this->Filter1)
  {
    this->Filter1->DebugOn();
  }
}

//----------------------------------------------------------------------------
void vtkImageOpenClose3D::DebugOff()
{
  this->vtkObject::DebugOff();
  if (this->Filter0)
  {
    this->Filter0->DebugOff();
  }
  if (this->Filter1)
  {
    this->Filter1->DebugOff();
  }
}

//----------------------------------------------------------------------------
// Pass modified message to sub filters.
void vtkImageOpenClose3D::Modified()
{
  this->vtkObject::Modified();
  if (this->Filter0)
  {
    this->Filter0->Modified();
  }

  if (this->Filter1)
  {
    this->Filter1->Modified();
  }
}


//----------------------------------------------------------------------------
// This method considers the sub filters MTimes when computing this objects
// MTime
vtkMTimeType vtkImageOpenClose3D::GetMTime()
{
  vtkMTimeType t1, t2;

  t1 = this->Superclass::GetMTime();
  if (this->Filter0)
  {
    t2 = this->Filter0->GetMTime();
    if (t2 > t1)
    {
      t1 = t2;
    }
  }
  if (this->Filter1)
  {
    t2 = this->Filter1->GetMTime();
    if (t2 > t1)
    {
      t1 = t2;
    }
  }

  return t1;
}

//----------------------------------------------------------------------------
int
vtkImageOpenClose3D::ComputePipelineMTime(vtkInformation* request,
                                          vtkInformationVector** inInfoVec,
                                          vtkInformationVector* outInfoVec,
                                          int requestFromOutputPort,
                                          vtkMTimeType* mtime)
{
  // Process the request on the internal pipeline.  Share our input
  // information with the first filter and our output information with
  // the last filter.
  vtkExecutive* exec0 = this->Filter0->GetExecutive();
  vtkExecutive* exec1 = this->Filter1->GetExecutive();
  exec0->SetSharedInputInformation(inInfoVec);
  exec1->SetSharedOutputInformation(outInfoVec);
  vtkMTimeType mtime1;
  if(exec1->ComputePipelineMTime(request,
                                 exec1->GetInputInformation(),
                                 exec1->GetOutputInformation(),
                                 requestFromOutputPort, &mtime1))
  {
    // Now run the request in this algorithm.
    return this->Superclass::ComputePipelineMTime(request,
                                                  inInfoVec, outInfoVec,
                                                  requestFromOutputPort,
                                                  mtime);
  }
  else
  {
    // The internal pipeline failed to process the request.
    vtkErrorMacro("Internal pipeline failed to process pipeline modified "
                  "time request.");
    return 0;
  }
}

//----------------------------------------------------------------------------
int vtkImageOpenClose3D::ProcessRequest(vtkInformation* request,
                                        vtkInformationVector** inInfoVec,
                                        vtkInformationVector* outInfoVec)
{
  // Process the request on the internal pipeline.  Share our input
  // information with the first filter and our output information with
  // the last filter.
  vtkExecutive* exec0 = this->Filter0->GetExecutive();
  vtkExecutive* exec1 = this->Filter1->GetExecutive();
  exec0->SetSharedInputInformation(inInfoVec);
  exec1->SetSharedOutputInformation(outInfoVec);
  return exec1->ProcessRequest(request,
                               exec1->GetInputInformation(),
                               exec1->GetOutputInformation());
}

//----------------------------------------------------------------------------
// Selects the size of gaps or objects removed.
void vtkImageOpenClose3D::SetKernelSize(int size0, int size1, int size2)
{
  if ( ! this->Filter0 || ! this->Filter1)
  {
    vtkErrorMacro(<< "SetKernelSize: Sub filter not created yet.");
    return;
  }

  this->Filter0->SetKernelSize(size0, size1, size2);
  this->Filter1->SetKernelSize(size0, size1, size2);
  // Sub filters take care of modified.
}

//----------------------------------------------------------------------------
// Determines the value that will closed.
// Close value is first dilated, and then eroded
void vtkImageOpenClose3D::SetCloseValue(double value)
{
  if ( ! this->Filter0 || ! this->Filter1)
  {
    vtkErrorMacro(<< "SetCloseValue: Sub filter not created yet.");
    return;
  }

  this->Filter0->SetDilateValue(value);
  this->Filter1->SetErodeValue(value);
}

//----------------------------------------------------------------------------
double vtkImageOpenClose3D::GetCloseValue()
{
  if ( ! this->Filter0)
  {
    vtkErrorMacro(<< "GetCloseValue: Sub filter not created yet.");
    return 0.0;
  }

  return this->Filter0->GetDilateValue();
}

//----------------------------------------------------------------------------
// Determines the value that will opened.
// Open value is first eroded, and then dilated.
void vtkImageOpenClose3D::SetOpenValue(double value)
{
  if ( ! this->Filter0 || ! this->Filter1)
  {
    vtkErrorMacro(<< "SetOpenValue: Sub filter not created yet.");
    return;
  }

  this->Filter0->SetErodeValue(value);
  this->Filter1->SetDilateValue(value);
}

//----------------------------------------------------------------------------
double vtkImageOpenClose3D::GetOpenValue()
{
  if ( ! this->Filter0)
  {
    vtkErrorMacro(<< "GetOpenValue: Sub filter not created yet.");
    return 0.0;
  }

  return this->Filter0->GetErodeValue();
}

//----------------------------------------------------------------------------
void vtkImageOpenClose3D::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  // These filters share our input and are therefore involved in a
  // reference loop.
  vtkGarbageCollectorReport(collector, this->Filter0, "Filter0");
  vtkGarbageCollectorReport(collector, this->Filter1, "Filter1");
}
