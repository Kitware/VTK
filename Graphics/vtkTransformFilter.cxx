/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransformFilter.h"

#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLinearTransform.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"

vtkStandardNewMacro(vtkTransformFilter);
vtkCxxSetObjectMacro(vtkTransformFilter,Transform,vtkAbstractTransform);

vtkTransformFilter::vtkTransformFilter()
{
  this->Transform = NULL;
}

vtkTransformFilter::~vtkTransformFilter()
{
  this->SetTransform(NULL);
}

int vtkTransformFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet *input = vtkPointSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPointSet *output = vtkPointSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPoints *inPts;
  vtkPoints *newPts;
  vtkDataArray *inVectors, *inCellVectors;;
  vtkFloatArray *newVectors=NULL, *newCellVectors=NULL;
  vtkDataArray *inNormals, *inCellNormals;
  vtkFloatArray *newNormals=NULL, *newCellNormals=NULL;
  vtkIdType numPts, numCells;
  vtkPointData *pd=input->GetPointData(), *outPD=output->GetPointData();
  vtkCellData *cd=input->GetCellData(), *outCD=output->GetCellData();

  vtkDebugMacro(<<"Executing transform filter");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  // Check input
  //
  if ( this->Transform == NULL )
    {
    vtkErrorMacro(<<"No transform defined!");
    return 1;
    }

  inPts = input->GetPoints();
  inVectors = pd->GetVectors();
  inNormals = pd->GetNormals();
  inCellVectors = cd->GetVectors();
  inCellNormals = cd->GetNormals();

  if ( !inPts )
    {
    vtkErrorMacro(<<"No input data");
    return 1;
    }

  numPts = inPts->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();

  newPts = vtkPoints::New();
  newPts->Allocate(numPts);
  if ( inVectors ) 
    {
    newVectors = vtkFloatArray::New();
    newVectors->SetNumberOfComponents(3);
    newVectors->Allocate(3*numPts);
    newVectors->SetName(inVectors->GetName());
    }
  if ( inNormals ) 
    {
    newNormals = vtkFloatArray::New();
    newNormals->SetNumberOfComponents(3);
    newNormals->Allocate(3*numPts);
    newNormals->SetName(inNormals->GetName());
    }

  this->UpdateProgress (.2);
  // Loop over all points, updating position
  //

  if ( inVectors || inNormals )
    {
    this->Transform->TransformPointsNormalsVectors(inPts,newPts,
                                                   inNormals,newNormals,
                                                   inVectors,newVectors);
    }
  else
    {
    this->Transform->TransformPoints(inPts,newPts);
    }  

  this->UpdateProgress (.6);

  // Can only transform cell normals/vectors if the transform
  // is linear.
  vtkLinearTransform* lt=vtkLinearTransform::SafeDownCast(this->Transform);
  if (lt)
    {
    if ( inCellVectors ) 
      {
      newCellVectors = vtkFloatArray::New();
      newCellVectors->SetNumberOfComponents(3);
      newCellVectors->Allocate(3*numCells);
      newCellVectors->SetName( inCellVectors->GetName() );
      lt->TransformVectors(inCellVectors,newCellVectors);
      }
    if ( inCellNormals ) 
      {
      newCellNormals = vtkFloatArray::New();
      newCellNormals->SetNumberOfComponents(3);
      newCellNormals->Allocate(3*numCells);
      newCellNormals->SetName( inCellNormals->GetName() );
      lt->TransformNormals(inCellNormals,newCellNormals);
      }
    }

  this->UpdateProgress (.8);

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();

  if (newNormals)
    {
    outPD->SetNormals(newNormals);
    newNormals->Delete();
    outPD->CopyNormalsOff();
    }

  if (newVectors)
    {
    outPD->SetVectors(newVectors);
    newVectors->Delete();
    outPD->CopyVectorsOff();
    }

  if (newCellNormals)
    {
    outCD->SetNormals(newCellNormals);
    newCellNormals->Delete();
    outCD->CopyNormalsOff();
    }

  if (newCellVectors)
    {
    outCD->SetVectors(newCellVectors);
    newCellVectors->Delete();
    outCD->CopyVectorsOff();
    }

  outPD->PassData(pd);
  outCD->PassData(cd);

  vtkFieldData* inFD = input->GetFieldData();
  if (inFD)
    {
    vtkFieldData* outFD = output->GetFieldData();
    if (!outFD)
      {
      outFD = vtkFieldData::New();
      output->SetFieldData(outFD);
      // We can still use outFD since it's registered
      // by the output
      outFD->Delete();
      }
    outFD->PassData(inFD);
    }

  return 1;
}

unsigned long vtkTransformFilter::GetMTime()
{
  unsigned long mTime=this->MTime.GetMTime();
  unsigned long transMTime;

  if ( this->Transform )
    {
    transMTime = this->Transform->GetMTime();
    mTime = ( transMTime > mTime ? transMTime : mTime );
    }

  return mTime;
}

void vtkTransformFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Transform: " << this->Transform << "\n";
}
