/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoundedPointSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBoundedPointSource.h"

#include "vtkCellArray.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkFloatArray.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkBoundedPointSource);

//----------------------------------------------------------------------------
vtkBoundedPointSource::vtkBoundedPointSource()
{
  this->NumberOfPoints = 100;

  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = -1.0;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = 1.0;

  this->OutputPointsPrecision = SINGLE_PRECISION;

  this->ProduceCellOutput = false;

  this->ProduceRandomScalars = false;
  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
int vtkBoundedPointSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType ptId;
  double x[3];

  vtkPoints *newPoints = vtkPoints::New();
  // Set the desired precision for the points in the output.
  if(this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPoints->SetDataType(VTK_DOUBLE);
  }
  else
  {
    newPoints->SetDataType(VTK_FLOAT);
  }

  // Generate the points
  newPoints->SetNumberOfPoints(this->NumberOfPoints);
  double xmin = (this->Bounds[0] < this->Bounds[1] ? this->Bounds[0] : this->Bounds[1]);
  double xmax = (this->Bounds[0] < this->Bounds[1] ? this->Bounds[1] : this->Bounds[0]);
  double ymin = (this->Bounds[2] < this->Bounds[3] ? this->Bounds[2] : this->Bounds[3]);
  double ymax = (this->Bounds[2] < this->Bounds[3] ? this->Bounds[3] : this->Bounds[2]);
  double zmin = (this->Bounds[4] < this->Bounds[5] ? this->Bounds[4] : this->Bounds[5]);
  double zmax = (this->Bounds[4] < this->Bounds[5] ? this->Bounds[5] : this->Bounds[4]);

  vtkMath *math = vtkMath::New();
  for (ptId=0; ptId<this->NumberOfPoints; ptId++)
  {
    x[0] = math->Random(xmin,xmax);
    x[1] = math->Random(ymin,ymax);
    x[2] = math->Random(zmin,zmax);
    newPoints->SetPoint(ptId,x);
  }
  output->SetPoints(newPoints);
  newPoints->Delete();

  // Generate the scalars if requested
  if ( this->ProduceRandomScalars )
  {
    vtkFloatArray *scalars = vtkFloatArray::New();
    scalars->SetName("RandomScalars");
    scalars->SetNumberOfTuples(this->NumberOfPoints);
    float *s = static_cast<float*>(scalars->GetVoidPointer(0));
    double sMin = (this->ScalarRange[0] < this->ScalarRange[1] ?
                   this->ScalarRange[0] : this->ScalarRange[1]);
    double sMax = (this->ScalarRange[0] < this->ScalarRange[1] ?
                   this->ScalarRange[1] : this->ScalarRange[0]);
    for (ptId=0; ptId<this->NumberOfPoints; ptId++)
    {
      *s++ = math->Random(sMin,sMax);
    }
    output->GetPointData()->SetScalars(scalars);
    scalars->Delete();
  }

  // Generate the vertices if requested
  if ( this->ProduceCellOutput )
  {
    vtkCellArray *newVerts = vtkCellArray::New();
    newVerts->Allocate(newVerts->EstimateSize(1,this->NumberOfPoints));
    newVerts->InsertNextCell(this->NumberOfPoints);
    for (ptId=0; ptId<this->NumberOfPoints; ptId++)
    {
      newVerts->InsertCellPoint(ptId);
    }
    output->SetVerts(newVerts);
    newVerts->Delete();
  }

  math->Delete();
  return 1;
}

//----------------------------------------------------------------------------
void vtkBoundedPointSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number Of Points: " << this->NumberOfPoints << "\n";

  for(int i=0;i<6;i++)
  {
    os << indent << "Bounds[" << i << "]: " << this->Bounds[i] << "\n";
  }

  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";

  os << indent << "Produce Cell Output: "
     << (this->ProduceCellOutput ? "On\n" : "Off\n");

  os << indent << "Produce Random Scalars: "
     << (this->ProduceRandomScalars ? "On\n" : "Off\n");
  os << indent << "Scalar Range (" << this->ScalarRange[0] << ","
     << this->ScalarRange[1] << ")\n";

}
