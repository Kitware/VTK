/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeDataObjectFilter.cxx
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
#include "vtkMergeDataObjectFilter.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkMergeDataObjectFilter, "1.16");
vtkStandardNewMacro(vtkMergeDataObjectFilter);

//----------------------------------------------------------------------------
// Create object with no input or output.
vtkMergeDataObjectFilter::vtkMergeDataObjectFilter()
{
  this->OutputField = VTK_DATA_OBJECT_FIELD;
}

//----------------------------------------------------------------------------
vtkMergeDataObjectFilter::~vtkMergeDataObjectFilter()
{
}

//----------------------------------------------------------------------------
// Specify a data object at a specified table location.
void vtkMergeDataObjectFilter::SetDataObject(vtkDataObject *d)
{
  this->vtkProcessObject::SetNthInput(1, d);
}

//----------------------------------------------------------------------------
// Get a pointer to a data object at a specified table location.
vtkDataObject *vtkMergeDataObjectFilter::GetDataObject()
{
  if (this->NumberOfInputs < 2)
    {
    return NULL;
    }
  else
    {
    return this->Inputs[1];
    }
}


//----------------------------------------------------------------------------
// Merge it all together
void vtkMergeDataObjectFilter::Execute()
{
  vtkDataObject *dataObject=this->GetDataObject();
  vtkFieldData *fd;
  vtkDataSet *input=this->GetInput();
  vtkDataSet *output=this->GetOutput();
  
  vtkDebugMacro(<<"Merging dataset and data object");

  if (dataObject == NULL)
    {
    vtkErrorMacro(<< "Data Object's Field Data is NULL.");
    return;
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
      return;
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
      return;
      }
    for(int i=0; i<fd->GetNumberOfArrays(); i++)
      {
      output->GetPointData()->AddArray(fd->GetArray(i));
      }
    }
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

