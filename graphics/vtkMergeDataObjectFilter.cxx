/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeDataObjectFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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
  vtkFieldData *fd=dataObject->GetFieldData();
  vtkDataSet *input=this->GetInput();
  vtkDataSet *output=this->GetOutput();
  
  vtkDebugMacro(<<"Merging dataset and data object");

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
    output->GetCellData()->SetFieldData(fd);
    }
  else if ( this->OutputField == VTK_POINT_DATA_FIELD )
    {
    int npts=fd->GetNumberOfTuples();
    if ( npts != input->GetNumberOfPoints() )
      {
      vtkErrorMacro(<<"Field data size incompatible with number of points");
      return;
      }
    output->GetPointData()->SetFieldData(fd);
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

