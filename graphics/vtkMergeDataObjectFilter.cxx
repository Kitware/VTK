/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeDataObjectFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkMergeDataObjectFilter.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkMergeDataObjectFilter* vtkMergeDataObjectFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMergeDataObjectFilter");
  if(ret)
    {
    return (vtkMergeDataObjectFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMergeDataObjectFilter;
}




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
  else //( this->OutputField == VTK_DATA_OBJECT_FIELD )
    {
    output->SetFieldData(fd);
    }
}


//----------------------------------------------------------------------------
void vtkMergeDataObjectFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

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

