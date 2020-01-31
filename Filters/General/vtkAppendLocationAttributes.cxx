/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendLocationAttributes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAppendLocationAttributes.h"

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellCenters.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkAppendLocationAttributes);

//----------------------------------------------------------------------------
// Generate points
int vtkAppendLocationAttributes::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the input and output
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]);
  vtkDataSet* output = vtkDataSet::GetData(outputVector);

  output->ShallowCopy(input);

  // Create cell centers array
  vtkNew<vtkDoubleArray> cellCenterArray;
  if (this->AppendCellCenters)
  {
    vtkIdType numCells = input->GetNumberOfCells();
    cellCenterArray->SetName("CellCenters");
    cellCenterArray->SetNumberOfComponents(3);
    cellCenterArray->SetNumberOfTuples(numCells);

    vtkCellCenters::ComputeCellCenters(input, cellCenterArray);

    vtkCellData* outCD = output->GetCellData();
    outCD->AddArray(cellCenterArray);

    this->UpdateProgress(0.66);
  }

  if (this->AppendPointLocations)
  {
    vtkPointData* outPD = output->GetPointData();
    vtkPointSet* outPointSet = vtkPointSet::SafeDownCast(output);
    if (outPointSet && outPointSet->GetPoints())
    {
      // Access point data array and shallow copy it to a point data array
      vtkDataArray* pointArray = outPointSet->GetPoints()->GetData();
      vtkSmartPointer<vtkDataArray> arrayCopy;
      arrayCopy.TakeReference(pointArray->NewInstance());
      arrayCopy->ShallowCopy(pointArray);
      arrayCopy->SetName("PointLocations");
      outPD->AddArray(arrayCopy);
    }
    else
    {
      // Use slower API to get point positions
      vtkNew<vtkDoubleArray> pointArray;
      pointArray->SetName("PointLocations");
      pointArray->SetNumberOfComponents(3);
      vtkIdType numPoints = input->GetNumberOfPoints();
      pointArray->SetNumberOfTuples(numPoints);
      for (vtkIdType id = 0; id < numPoints; ++id)
      {
        double x[3];
        input->GetPoint(id, x);
        pointArray->SetTypedTuple(id, x);
      }
      outPD->AddArray(pointArray);
    }
  }

  this->UpdateProgress(1.0);
  return 1;
}

//----------------------------------------------------------------------------
int vtkAppendLocationAttributes::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkAppendLocationAttributes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AppendPointLocations: " << (this->AppendPointLocations ? "On\n" : "Off\n");
  os << indent << "AppendCellCenters: " << (this->AppendCellCenters ? "On" : "Off") << endl;
}
