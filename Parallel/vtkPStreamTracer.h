/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPStreamTracer.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPStreamTracer - Abstract superclass for parallel streamline generators
// .SECTION Description
// This class implements some necessary functionality used by distributed
// and parallel streamline generators. Note that all processes must have
// access to the WHOLE seed source, i.e. the source must be identical
// on all processes.
// .SECTION See Also
// vtkStreamTracer vtkDistributedStreamTracer vtkMPIStreamTracer

#ifndef __vtkPStreamTracer_h
#define __vtkPStreamTracer_h

#include "vtkStreamTracer.h"

#include "vtkSmartPointer.h"

#include <vector>

class vtkInterpolatedVelocityField;
class vtkMultiProcessController;

class VTK_PARALLEL_EXPORT vtkPStreamTracer : public vtkStreamTracer
{
public:
  vtkTypeRevisionMacro(vtkPStreamTracer,vtkStreamTracer);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the controller use in compositing (set to
  // the global controller by default)
  // If not using the default, this must be called before any
  // other methods.
  virtual void SetController(vtkMultiProcessController* controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // If you want to generate traces using an arbitrary vector array, 
  // then set its name here. By default this in NULL and the filter will 
  // use the active vector array.
  vtkGetStringMacro(InputVectorsSelection);
  void SelectInputVectors(const char *fieldName) 
    {this->SetInputVectorsSelection(fieldName);}
  
protected:

  vtkPStreamTracer();
  ~vtkPStreamTracer();

  virtual void Execute();
  virtual void ExecuteInformation();
  virtual void ComputeInputUpdateExtents( vtkDataObject *output );

  vtkMultiProcessController* Controller;

  vtkInterpolatedVelocityField* Interpolator;
  void SetInterpolator(vtkInterpolatedVelocityField*);

  // See the implementation for comments
  void SendCellPoint(vtkPolyData* data,
                     vtkIdType streamId, 
                     vtkIdType idx, 
                     int sendToId);
  void ReceiveCellPoint(vtkPolyData* tomod, int streamId, vtkIdType idx);
  void SendFirstPoints();
  void ReceiveLastPoints();
  void MoveToNextSend();

  virtual void ParallelIntegrate() = 0;

  vtkDataArray* Seeds;
  vtkIdList* SeedIds;
  vtkIntArray* IntegrationDirections;

//BTX
  typedef vtkstd::vector< vtkSmartPointer<vtkPolyData> > TmpOutputsType;
//ETX

  TmpOutputsType TmpOutputs;

private:
  vtkPStreamTracer(const vtkPStreamTracer&);  // Not implemented.
  void operator=(const vtkPStreamTracer&);  // Not implemented.
};


#endif


