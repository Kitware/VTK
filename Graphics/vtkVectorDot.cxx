/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorDot.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVectorDot.h"

#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkVectorDot);

// Construct object with scalar range is (-1,1).
vtkVectorDot::vtkVectorDot()
{
  this->ScalarRange[0] = -1.0;
  this->ScalarRange[1] = 1.0;
}

//
// Compute dot product.
//
int vtkVectorDot::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType ptId, numPts;
  vtkFloatArray *newScalars;
  vtkDataArray *inNormals;
  vtkDataArray *inVectors;
  double s, n[3], v[3], min, max, dR, dS;
  vtkPointData *pd=input->GetPointData(), *outPD=output->GetPointData();

  // Initialize
  //
  vtkDebugMacro(<<"Generating vector/normal dot product!");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if ( (numPts=input->GetNumberOfPoints()) < 1 )
    {
    vtkErrorMacro(<< "No points!");
    return 1;
    }
  if ( (inVectors=pd->GetVectors()) == NULL )
    {
    vtkErrorMacro(<< "No vectors defined!");
    return 1;
    }
  if ( (inNormals=pd->GetNormals()) == NULL )
    {
    vtkErrorMacro(<< "No normals defined!");
    return 1;
    }

  // Allocate
  //
  newScalars = vtkFloatArray::New();
  newScalars->Allocate(numPts);

  // Compute initial scalars
  //
  int abort=0;
  vtkIdType progressInterval=numPts/20 + 1;
  for (min=VTK_DOUBLE_MAX,max=(-VTK_DOUBLE_MAX),ptId=0; 
       ptId < numPts && !abort; ptId++)
    {
    if ( ! (ptId % progressInterval) ) 
      {
      this->UpdateProgress ((double)ptId/numPts);
      abort = this->GetAbortExecute();
      }
    inNormals->GetTuple(ptId, n);
    inVectors->GetTuple(ptId, v);
    s = vtkMath::Dot(n,v);
    if ( s < min )
      {
      min = s;
      }
    if ( s > max )
      {
      max = s;
      }
    newScalars->InsertTuple(ptId,&s);
    }

  // Map scalars into scalar range
  //
  if ( (dR=this->ScalarRange[1]-this->ScalarRange[0]) == 0.0 )
    {
    dR = 1.0;
    }
  if ( (dS=max-min) == 0.0 )
    {
    dS = 1.0;
    }

  for ( ptId=0; ptId < numPts; ptId++ )
    {
    s = newScalars->GetComponent(ptId,0);
    s = ((s - min)/dS) * dR + this->ScalarRange[0];
    newScalars->InsertTuple(ptId,&s);
    }

  // Update self and relase memory
  //
  outPD->PassData(input->GetPointData());

  int idx = outPD->AddArray(newScalars);
  outPD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
  newScalars->Delete();

  return 1;
}

void vtkVectorDot::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Scalar Range: (" << this->ScalarRange[0] << ", "
                                    << this->ScalarRange[1] << ")\n";
}
