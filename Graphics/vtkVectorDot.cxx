/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorDot.cxx
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
#include "vtkVectorDot.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"

vtkCxxRevisionMacro(vtkVectorDot, "1.33");
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
void vtkVectorDot::Execute()
{
  vtkIdType ptId, numPts;
  vtkFloatArray *newScalars;
  vtkDataSet *input = this->GetInput();
  vtkDataArray *inNormals;
  vtkDataArray *inVectors;
  float s, *n, *v, min, max, dR, dS;
  vtkDataSet *output = this->GetOutput();
  vtkPointData *pd=input->GetPointData(), *outPD=output->GetPointData();

  // Initialize
  //
  vtkDebugMacro(<<"Generating vector/normal dot product!");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if ( (numPts=input->GetNumberOfPoints()) < 1 )
    {
    vtkErrorMacro(<< "No points!");
    return;
    }
  if ( (inVectors=pd->GetVectors()) == NULL )
    {
    vtkErrorMacro(<< "No vectors defined!");
    return;
    }
  if ( (inNormals=pd->GetNormals()) == NULL )
    {
    vtkErrorMacro(<< "No normals defined!");
    return;
    }

  // Allocate
  //
  newScalars = vtkFloatArray::New();
  newScalars->Allocate(numPts);

  // Compute initial scalars
  //
  int abort=0;
  vtkIdType progressInterval=numPts/20 + 1;
  for (min=VTK_LARGE_FLOAT,max=(-VTK_LARGE_FLOAT),ptId=0; 
       ptId < numPts && !abort; ptId++)
    {
    if ( ! (ptId % progressInterval) ) 
      {
      this->UpdateProgress ((float)ptId/numPts);
      abort = this->GetAbortExecute();
      }
    n = inNormals->GetTuple(ptId);
    v = inVectors->GetTuple(ptId);
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
  outPD->CopyScalarsOff();
  outPD->PassData(input->GetPointData());

  outPD->SetScalars(newScalars);
  newScalars->Delete();
}

void vtkVectorDot::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Scalar Range: (" << this->ScalarRange[0] << ", "
                                    << this->ScalarRange[1] << ")\n";
}
