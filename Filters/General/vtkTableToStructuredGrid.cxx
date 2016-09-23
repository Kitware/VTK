/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableToStructuredGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTableToStructuredGrid.h"

#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"

vtkStandardNewMacro(vtkTableToStructuredGrid);
//----------------------------------------------------------------------------
vtkTableToStructuredGrid::vtkTableToStructuredGrid()
{
  this->XColumn = 0;
  this->YColumn = 0;
  this->ZColumn = 0;
  this->XComponent = 0;
  this->YComponent = 0;
  this->ZComponent = 0;
  this->WholeExtent[0] = this->WholeExtent[1] = this->WholeExtent[2] =
    this->WholeExtent[3] = this->WholeExtent[4] = this->WholeExtent[5] = 0;
}

//----------------------------------------------------------------------------
vtkTableToStructuredGrid::~vtkTableToStructuredGrid()
{
  this->SetXColumn(0);
  this->SetYColumn(0);
  this->SetZColumn(0);
}

//----------------------------------------------------------------------------
int vtkTableToStructuredGrid::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

// ----------------------------------------------------------------------------
int vtkTableToStructuredGrid::RequestInformation(
   vtkInformation *vtkNotUsed(request),
   vtkInformationVector **vtkNotUsed(inputVector),
   vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               this->WholeExtent, 6);

  return 1;
}


//----------------------------------------------------------------------------
int vtkTableToStructuredGrid::RequestData(
  vtkInformation* vtkNotUsed(request), vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkStructuredGrid* output = vtkStructuredGrid::GetData(outputVector, 0);
  vtkTable* input = vtkTable::GetData(inputVector[0], 0);

  vtkStreamingDemandDrivenPipeline *sddp =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  int extent[6];
  sddp->GetOutputInformation(0)->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent);
  return this->Convert(input, output, extent);
}

//----------------------------------------------------------------------------
int vtkTableToStructuredGrid::Convert(vtkTable* input,
  vtkStructuredGrid* output, int extent[6])
{
  int num_values = (extent[1] - extent[0] + 1) *
    (extent[3] - extent[2] + 1) *
    (extent[5] - extent[4] + 1);

  if (input->GetNumberOfRows() != num_values)
  {
    vtkErrorMacro("The input table must have exactly " << num_values
      << " rows. Currently it has " << input->GetNumberOfRows() << " rows.");
    return 0;
  }

  vtkDataArray* xarray = vtkArrayDownCast<vtkDataArray>(
    input->GetColumnByName(this->XColumn));
  vtkDataArray* yarray = vtkArrayDownCast<vtkDataArray>(
    input->GetColumnByName(this->YColumn));
  vtkDataArray* zarray = vtkArrayDownCast<vtkDataArray>(
    input->GetColumnByName(this->ZColumn));
  if (!xarray || !yarray || !zarray)
  {
    vtkErrorMacro("Failed to locate  the columns to use for the point"
      " coordinates");
    return 0;
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
    for (vtkIdType cc=0; cc < numtuples; cc++)
    {
      newData->SetComponent(cc, 0, xarray->GetComponent(cc, this->XComponent));
      newData->SetComponent(cc, 1, yarray->GetComponent(cc, this->YComponent));
      newData->SetComponent(cc, 2, zarray->GetComponent(cc, this->ZComponent));
    }
    newPoints->SetData(newData);
    newData->Delete();
  }

  output->SetExtent(extent);
  output->SetPoints(newPoints);
  newPoints->Delete();

  // Add all other columns as point data.
  for (int cc=0; cc < input->GetNumberOfColumns(); cc++)
  {
    vtkAbstractArray* arr = input->GetColumn(cc);
    if (arr != xarray && arr != yarray && arr != zarray)
    {
      output->GetPointData()->AddArray(arr);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkTableToStructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "WholeExtent: "
    << this->WholeExtent[0] << ", "
    << this->WholeExtent[1] << ", "
    << this->WholeExtent[2] << ", "
    << this->WholeExtent[3] << ", "
    << this->WholeExtent[4] << ", "
    << this->WholeExtent[5] << endl;
  os << indent << "XColumn: "
    << (this->XColumn? this->XColumn : "(none)") << endl;
  os << indent << "XComponent: " << this->XComponent << endl;
  os << indent << "YColumn: "
    << (this->YColumn? this->YColumn : "(none)") << endl;
  os << indent << "YComponent: " << this->YComponent << endl;
  os << indent << "ZColumn: "
    << (this->ZColumn? this->ZColumn : "(none)") << endl;
  os << indent << "ZComponent: " << this->ZComponent << endl;
}

