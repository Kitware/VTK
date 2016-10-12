/*=========================================================================

  Program:   ParaView
  Module:    vtkTableToPolyData.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTableToPolyData.h"

#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkTable.h"
#include "vtkCellArray.h"
#include "vtkInformation.h"

vtkStandardNewMacro(vtkTableToPolyData);
//----------------------------------------------------------------------------
vtkTableToPolyData::vtkTableToPolyData()
{
  this->XColumn = 0;
  this->YColumn = 0;
  this->ZColumn = 0;
  this->XColumnIndex = -1;
  this->YColumnIndex = -1;
  this->ZColumnIndex = -1;
  this->XComponent = 0;
  this->YComponent = 0;
  this->ZComponent = 0;
  this->Create2DPoints = 0;
  this->PreserveCoordinateColumnsAsDataArrays = false;
}

//----------------------------------------------------------------------------
vtkTableToPolyData::~vtkTableToPolyData()
{
  this->SetXColumn(0);
  this->SetYColumn(0);
  this->SetZColumn(0);
}

//----------------------------------------------------------------------------
int vtkTableToPolyData::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

//----------------------------------------------------------------------------
int vtkTableToPolyData::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkTable* input = vtkTable::GetData(inputVector[0], 0);
  vtkPolyData* output = vtkPolyData::GetData(outputVector, 0);

  if (input->GetNumberOfRows() == 0)
  {
    // empty input.
    return 1;
  }

  vtkDataArray* xarray = NULL;
  vtkDataArray* yarray = NULL;
  vtkDataArray* zarray = NULL;


  if(this->XColumn && this->YColumn)
  {
    xarray = vtkArrayDownCast<vtkDataArray>(
      input->GetColumnByName(this->XColumn));
    yarray = vtkArrayDownCast<vtkDataArray>(
      input->GetColumnByName(this->YColumn));
    zarray = vtkArrayDownCast<vtkDataArray>(
      input->GetColumnByName(this->ZColumn));
  }
  else if(this->XColumnIndex >= 0)
  {
    xarray = vtkArrayDownCast<vtkDataArray>(
      input->GetColumn(this->XColumnIndex));
    yarray = vtkArrayDownCast<vtkDataArray>(
      input->GetColumn(this->YColumnIndex));
    zarray = vtkArrayDownCast<vtkDataArray>(
      input->GetColumn(this->ZColumnIndex));
  }

  // zarray is optional
  if(this->Create2DPoints)
  {
    if (!xarray || !yarray)
    {
      vtkErrorMacro("Failed to locate  the columns to use for the point"
        " coordinates");
      return 0;
    }
  }
  else
  {
    if (!xarray || !yarray || !zarray)
    {
      vtkErrorMacro("Failed to locate  the columns to use for the point"
        " coordinates");
      return 0;
    }
  }

  vtkPoints* newPoints = vtkPoints::New();

  if (xarray == yarray && yarray == zarray &&
    this->XComponent == 0 &&
    this->YComponent == 1 &&
    this->ZComponent == 2 &&
    xarray->GetNumberOfComponents() == 3)
  {
    newPoints->SetData(xarray);
  }
  else
  {
    // Ideally we determine the smallest data type that can contain the values
    // in all the 3 arrays. For now I am just going with doubles.
    vtkDoubleArray* newData =  vtkDoubleArray::New();
    newData->SetNumberOfComponents(3);
    newData->SetNumberOfTuples(input->GetNumberOfRows());
    vtkIdType numtuples = newData->GetNumberOfTuples();
    if(this->Create2DPoints)
    {
      for (vtkIdType cc=0; cc < numtuples; cc++)
      {
        newData->SetComponent(cc, 0, xarray->GetComponent(cc, this->XComponent));
        newData->SetComponent(cc, 1, yarray->GetComponent(cc, this->YComponent));
        newData->SetComponent(cc, 2, 0.0);
      }
    }
    else
    {
      for (vtkIdType cc=0; cc < numtuples; cc++)
      {
        newData->SetComponent(cc, 0, xarray->GetComponent(cc, this->XComponent));
        newData->SetComponent(cc, 1, yarray->GetComponent(cc, this->YComponent));
        newData->SetComponent(cc, 2, zarray->GetComponent(cc, this->ZComponent));
      }
    }
    newPoints->SetData(newData);
    newData->Delete();
  }

  output->SetPoints(newPoints);
  newPoints->Delete();

  // Now create a poly-vertex cell will all the points.
  vtkIdType numPts = newPoints->GetNumberOfPoints();
  vtkIdType *ptIds = new vtkIdType[numPts];
  for (vtkIdType cc=0; cc < numPts; cc++)
  {
    ptIds[cc] = cc;
  }
  output->Allocate(1);
  output->InsertNextCell(VTK_POLY_VERTEX, numPts, ptIds);
  delete [] ptIds;

  // Add all other columns as point data.
  for (int cc=0; cc < input->GetNumberOfColumns(); cc++)
  {
    vtkAbstractArray* arr = input->GetColumn(cc);
    if(this->PreserveCoordinateColumnsAsDataArrays)
    {
      output->GetPointData()->AddArray(arr);
    }
    else if (arr != xarray && arr != yarray && arr != zarray)
    {
      output->GetPointData()->AddArray(arr);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkTableToPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "XColumn: "
    << (this->XColumn? this->XColumn : "(none)") << endl;
  os << indent << "XComponent: " << this->XComponent << endl;
  os << indent << "XColumnIndex: " << this->XColumnIndex << endl;
  os << indent << "YColumn: "
    << (this->YColumn? this->YColumn : "(none)") << endl;
  os << indent << "YComponent: " << this->YComponent << endl;
  os << indent << "YColumnIndex: " << this->YColumnIndex << endl;
  os << indent << "ZColumn: "
    << (this->ZColumn? this->ZColumn : "(none)") << endl;
  os << indent << "ZComponent: " << this->ZComponent << endl;
  os << indent << "ZColumnIndex: " << this->ZColumnIndex << endl;
  os << indent << "Create2DPoints: " << (this->Create2DPoints ? "true" : "false") << endl;
  os << indent << "PreserveCoordinateColumnsAsDataArrays: "
     << (this->PreserveCoordinateColumnsAsDataArrays ? "true" : "false") << endl;
}


