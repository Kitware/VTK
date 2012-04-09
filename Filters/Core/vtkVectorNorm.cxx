/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorNorm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVectorNorm.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

#include <math.h>

vtkStandardNewMacro(vtkVectorNorm);

// Construct with normalize flag off.
vtkVectorNorm::vtkVectorNorm()
{
  this->Normalize = 0;
  this->AttributeMode = VTK_ATTRIBUTE_MODE_DEFAULT;
}

int vtkVectorNorm::RequestData(
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

  vtkIdType numVectors, i;
  int computePtScalars=1, computeCellScalars=1;
  vtkFloatArray *newScalars;
  double v[3], s, maxScalar;
  vtkDataArray *ptVectors, *cellVectors;
  vtkPointData *pd=input->GetPointData(), *outPD=output->GetPointData();
  vtkCellData *cd=input->GetCellData(), *outCD=output->GetCellData();

  // Initialize
  vtkDebugMacro(<<"Computing norm of vectors!");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  ptVectors = pd->GetVectors();
  cellVectors = cd->GetVectors();
  if (!ptVectors || this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_CELL_DATA)
    {
    computePtScalars = 0;
    }

  if (!cellVectors || this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_POINT_DATA)
    {
    computeCellScalars = 0;
    }

  if ( !computeCellScalars && !computePtScalars )
    {
    vtkErrorMacro(<< "No vector norm to compute!");
    return 1;
    }

  // Allocate / operate on point data
  int abort=0;
  vtkIdType progressInterval;
  if ( computePtScalars )
    {
    numVectors = ptVectors->GetNumberOfTuples();
    newScalars = vtkFloatArray::New();
    newScalars->SetNumberOfTuples(numVectors);

    progressInterval=numVectors/10+1;
    for (maxScalar=0.0, i=0; i < numVectors && !abort; i++)
      {
      ptVectors->GetTuple(i, v);
      s = sqrt((double)v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
      if ( s > maxScalar )
        {
        maxScalar = s;
        }
      newScalars->SetComponent(i,0,s);

      if ( ! (i % progressInterval) ) 
        {
        vtkDebugMacro(<<"Computing point vector norm #" << i);
        this->UpdateProgress (0.5*i/numVectors);
        }
      }

    // If necessary, normalize
    if ( this->Normalize && maxScalar > 0.0 )
      {
      for (i=0; i < numVectors; i++)
        {
        s = newScalars->GetComponent(i,0);
        s /= maxScalar;
        newScalars->SetComponent(i,0,s);
        }
      }

    int idx = outPD->AddArray(newScalars);
    outPD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    newScalars->Delete();
    outPD->CopyScalarsOff();
    }//if computing point scalars

  // Allocate / operate on cell data
  if ( computeCellScalars )
    {
    numVectors = cellVectors->GetNumberOfTuples();
    newScalars = vtkFloatArray::New();
    newScalars->SetNumberOfTuples(numVectors);

    progressInterval=numVectors/10+1;
    for (maxScalar=0.0, i=0; i < numVectors && !abort; i++)
      {
      cellVectors->GetTuple(i, v);
      s = sqrt((double)v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
      if ( s > maxScalar )
        {
        maxScalar = s;
        }
      newScalars->SetComponent(i,0,s);
      if ( ! (i % progressInterval) ) 
        {
        vtkDebugMacro(<<"Computing cell vector norm #" << i);
        this->UpdateProgress (0.5+0.5*i/numVectors);
        }
      }

    // If necessary, normalize
    if ( this->Normalize && maxScalar > 0.0 )
      {
      for (i=0; i < numVectors; i++)
        {
        s = newScalars->GetComponent(i,0);
        s /= maxScalar;
        newScalars->SetComponent(i,0,s);
        }
      }

    int idx = outCD->AddArray(newScalars);
    outCD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    newScalars->Delete();
    outCD->CopyScalarsOff();
    }//if computing cell scalars

  // Pass appropriate data through to output
  outPD->PassData(pd);
  outCD->PassData(cd);

  return 1;
}

// Return the method for generating scalar data as a string.
const char *vtkVectorNorm::GetAttributeModeAsString(void)
{
  if ( this->AttributeMode == VTK_ATTRIBUTE_MODE_DEFAULT )
    {
    return "Default";
    }
  else if ( this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_POINT_DATA )
    {
    return "UsePointData";
    }
  else 
    {
    return "UseCellData";
    }
}

void vtkVectorNorm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Normalize: " << (this->Normalize ? "On\n" : "Off\n");
  os << indent << "Attribute Mode: " << this->GetAttributeModeAsString() 
     << endl;
}
