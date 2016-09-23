/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeDataObjectFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMergeDataObjectFilter.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkFieldData.h"
#include "vtkFieldDataToAttributeDataFilter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkMergeDataObjectFilter);

//----------------------------------------------------------------------------
// Create object with no input or output.
vtkMergeDataObjectFilter::vtkMergeDataObjectFilter()
{
  this->OutputField = VTK_DATA_OBJECT_FIELD;
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
vtkMergeDataObjectFilter::~vtkMergeDataObjectFilter()
{
}

//----------------------------------------------------------------------------
// Specify a data object at a specified table location.
void vtkMergeDataObjectFilter::SetDataObjectInputData(vtkDataObject *d)
{
  this->SetInputData(1, d);
}

//----------------------------------------------------------------------------
// Get a pointer to a data object at a specified table location.
vtkDataObject *vtkMergeDataObjectFilter::GetDataObject()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return NULL;
  }
  return this->GetExecutive()->GetInputData(1, 0);
}


//----------------------------------------------------------------------------
// Merge it all together
int vtkMergeDataObjectFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkInformation *dataObjectInfo = 0;
  if (this->GetNumberOfInputConnections(1) > 0)
  {
    dataObjectInfo = inputVector[1]->GetInformationObject(0);
  }

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataObject *dataObject=0;
  if (dataObjectInfo)
  {
    dataObject = dataObjectInfo->Get(vtkDataObject::DATA_OBJECT());
  }

  vtkFieldData *fd;

  vtkDebugMacro(<<"Merging dataset and data object");

  if (!dataObject)
  {
    vtkErrorMacro(<< "Data Object's Field Data is NULL.");
    return 1;
  }

  fd=dataObject->GetFieldData();

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if ( this->OutputField == VTK_CELL_DATA_FIELD )
  {
    int ncells=fd->GetNumberOfTuples();
    if ( ncells != input->GetNumberOfCells() )
    {
      vtkErrorMacro(<<"Field data size incompatible with number of cells");
      return 1;
    }
    for(int i=0; i<fd->GetNumberOfArrays(); i++)
    {
      output->GetCellData()->AddArray(fd->GetArray(i));
    }
  }
  else if ( this->OutputField == VTK_POINT_DATA_FIELD )
  {
    int npts=fd->GetNumberOfTuples();
    if ( npts != input->GetNumberOfPoints() )
    {
      vtkErrorMacro(<<"Field data size incompatible with number of points");
      return 1;
    }
    for(int i=0; i<fd->GetNumberOfArrays(); i++)
    {
      output->GetPointData()->AddArray(fd->GetArray(i));
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkMergeDataObjectFilter::SetOutputFieldToDataObjectField()
{
  this->SetOutputField(VTK_DATA_OBJECT_FIELD);
}

//----------------------------------------------------------------------------
void vtkMergeDataObjectFilter::SetOutputFieldToPointDataField()
{
  this->SetOutputField(VTK_POINT_DATA_FIELD);
}

//----------------------------------------------------------------------------
void vtkMergeDataObjectFilter::SetOutputFieldToCellDataField()
{
  this->SetOutputField(VTK_CELL_DATA_FIELD);
}

//----------------------------------------------------------------------------
int vtkMergeDataObjectFilter::FillInputPortInformation(int port,
                                                       vtkInformation *info)
{
  if (port == 0)
  {
    return this->Superclass::FillInputPortInformation(port, info);
  }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkMergeDataObjectFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Output Field: ";
  if ( this->OutputField == VTK_DATA_OBJECT_FIELD )
  {
    os << "DataObjectField\n";
  }
  else if ( this->OutputField == VTK_POINT_DATA_FIELD )
  {
    os << "PointDataField\n";
  }
  else //if ( this->OutputField == VTK_CELL_DATA_FIELD )
  {
    os << "CellDataField\n";
  }

}
