/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThresholdPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkThresholdPoints.h"

#include "vtkCellArray.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkThresholdPoints);

// Construct with lower threshold=0, upper threshold=1, and threshold
// function=upper.
vtkThresholdPoints::vtkThresholdPoints()
{
  this->LowerThreshold = 0.0;
  this->UpperThreshold = 1.0;
  this->OutputPointsPrecision = DEFAULT_PRECISION;

  this->ThresholdFunction = &vtkThresholdPoints::Upper;

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0,0,0, vtkDataObject::FIELD_ASSOCIATION_POINTS,
    vtkDataSetAttributes::SCALARS);
}

// Criterion is cells whose scalars are less than lower threshold.
void vtkThresholdPoints::ThresholdByLower(double lower)
{
  int isModified=0;

  if ( this->ThresholdFunction != &vtkThresholdPoints::Lower )
  {
    this->ThresholdFunction = &vtkThresholdPoints::Lower;
    isModified=1;
  }

  if ( this->LowerThreshold != lower )
  {
    this->LowerThreshold = lower;
    isModified=1;
  }

  if(isModified)
  {
    this->Modified();
  }
}

// Criterion is cells whose scalars are less than upper threshold.
void vtkThresholdPoints::ThresholdByUpper(double upper)
{
  int isModified=0;

  if ( this->ThresholdFunction != &vtkThresholdPoints::Upper )
  {
    this->ThresholdFunction = &vtkThresholdPoints::Upper;
    isModified=1;
  }

  if ( this->UpperThreshold != upper )
  {
    this->UpperThreshold = upper;
    isModified=1;
  }

  if(isModified)
  {
    this->Modified();
  }
}

// Criterion is cells whose scalars are between lower and upper thresholds.
void vtkThresholdPoints::ThresholdBetween(double lower, double upper)
{
  int isModified=0;

  if ( this->ThresholdFunction != &vtkThresholdPoints::Between )
  {
    this->ThresholdFunction = &vtkThresholdPoints::Between;
    isModified=1;
  }

  if ( this->LowerThreshold != lower )
  {
    this->LowerThreshold = lower;
    isModified=1;
  }

  if ( this->UpperThreshold != upper )
  {
    this->UpperThreshold = upper;
    isModified=1;
  }

  if(isModified)
  {
    this->Modified();
  }
}

int vtkThresholdPoints::RequestData(
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
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataArray *inScalars;
  vtkPoints *newPoints;
  vtkPointData *pd, *outPD;
  vtkCellArray *verts;
  vtkIdType ptId, numPts, pts[1];
  double x[3];

  vtkDebugMacro(<< "Executing threshold points filter");

  if ( ! (inScalars = this->GetInputArrayToProcess(0,inputVector) ) )
  {
    vtkErrorMacro(<<"No scalar data to threshold");
    return 1;
  }

  numPts = input->GetNumberOfPoints();

  if(numPts < 1)
  {
    vtkErrorMacro(<<"No points to threshold");
    return 1;
  }

  newPoints = vtkPoints::New();

  // Set the desired precision for the points in the output.
  if(this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    vtkPointSet *inputPointSet = vtkPointSet::SafeDownCast(input);
    if(inputPointSet)
    {
      newPoints->SetDataType(inputPointSet->GetPoints()->GetDataType());
    }
    else
    {
      newPoints->SetDataType(VTK_FLOAT);
    }
  }
  else if(this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPoints->SetDataType(VTK_FLOAT);
  }
  else if(this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPoints->SetDataType(VTK_DOUBLE);
  }

  newPoints->Allocate(numPts);
  pd = input->GetPointData();
  outPD = output->GetPointData();
  outPD->CopyAllocate(pd);
  verts = vtkCellArray::New();
  verts->Allocate(verts->EstimateSize(numPts,1));

  // Check that the scalars of each point satisfy the threshold criterion
  int abort=0;
  vtkIdType progressInterval = numPts/20+1;

  for (ptId=0; ptId < numPts && !abort; ptId++)
  {
    if ( !(ptId % progressInterval) )
    {
      this->UpdateProgress((double)ptId/numPts);
      abort = this->GetAbortExecute();
    }

    if ( (this->*(this->ThresholdFunction))(inScalars->GetComponent(ptId,0)) )
    {
      input->GetPoint(ptId, x);
      pts[0] = newPoints->InsertNextPoint(x);
      outPD->CopyData(pd,ptId,pts[0]);
      verts->InsertNextCell(1,pts);
    } // satisfied thresholding
  } // for all points

  vtkDebugMacro(<< "Extracted " << output->GetNumberOfPoints() << " points.");

  // Update ourselves and release memory
  //
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->SetVerts(verts);
  verts->Delete();

  output->Squeeze();

  return 1;
}

int vtkThresholdPoints::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

void vtkThresholdPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Lower Threshold: " << this->LowerThreshold << "\n";;
  os << indent << "Upper Threshold: " << this->UpperThreshold << "\n";;
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
