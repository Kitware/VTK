/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPistonSlice.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPistonSlice.h"

#include "vtkObjectFactory.h"
#include "vtkPistonDataObject.h"
#include "vtkPlane.h"

vtkStandardNewMacro(vtkPistonSlice);

namespace vtkpiston {
  // execution method found in vtkPistonSlice.cu
  void ExecutePistonSlice(vtkPistonDataObject *inData,
                          float* origin, float*normal, float offset,
                          vtkPistonDataObject *outData);
}

//----------------------------------------------------------------------------
vtkPistonSlice::vtkPistonSlice()
{
  this->Plane = vtkPlane::New();
  this->Offset = 0.0;
}

//----------------------------------------------------------------------------
vtkPistonSlice::~vtkPistonSlice()
{
  this->Plane->Delete();
}

//------------------------------------------------------------------------------
void vtkPistonSlice::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Offset: " << this->Offset << endl;
}

//----------------------------------------------------------------------------
void vtkPistonSlice::SetClippingPlane(vtkPlane *plane)
{
  if (this->Plane != plane)
  {
    this->Plane->Delete();
    this->Plane = plane;
    this->Plane->Register(this);
  }
}

//----------------------------------------------------------------------------
int vtkPistonSlice::ComputePipelineMTime(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* vtkNotUsed(outputVector),
  int vtkNotUsed(requestFromOutputPort),
  unsigned long* mtime)
{
  unsigned long mTime = this->GetMTime();

  if (this->Plane)
    {
    unsigned long planeMTime = this->Plane->GetMTime();
    if (planeMTime > mTime)
      {
      mTime = planeMTime;
      }
    }

  *mtime = mTime;

  return 1;
}

//----------------------------------------------------------------------------
int vtkPistonSlice::RequestData(vtkInformation *request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector)
{
  vtkPistonDataObject *id = vtkPistonDataObject::GetData(inputVector[0]);
  vtkPistonDataObject *od = vtkPistonDataObject::GetData(outputVector);
  this->PassBoundsForward(id,od);

  float *origin = new float[3];
  double *pval;
  pval = this->Plane->GetOrigin();
  origin[0] = pval[0];
  origin[1] = pval[1];
  origin[2] = pval[2];
  float *normal = new float[3];
  pval = this->Plane->GetNormal();
  normal[0] = pval[0];
  normal[1] = pval[1];
  normal[2] = pval[2];

  vtkpiston::ExecutePistonSlice(id, origin, normal, this->Offset, od);

  delete[] origin;
  delete[] normal;

  return 1;
}
