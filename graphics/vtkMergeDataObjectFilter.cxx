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

// Create object with no input or output.
vtkMergeDataObjectFilter::vtkMergeDataObjectFilter()
{
  this->DataObject = NULL;
  this->OutputField = VTK_DATA_OBJECT_FIELD;
}

vtkMergeDataObjectFilter::~vtkMergeDataObjectFilter()
{
  if (this->DataObject) {this->DataObject->UnRegister(this);}
  this->DataObject = NULL;
}

void vtkMergeDataObjectFilter::Update()
{
  // make sure output has been created
  if ( !this->Output )
    {
    vtkErrorMacro(<< "No output has been created...need to set input");
    return;
    }

  // make sure input is available
  if ( !this->Input )
    {
    vtkErrorMacro(<< "No input...can't execute!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating)
    {
    return;
    }

  this->Updating = 1;
  this->Input->Update();
  if ( this->DataObject )
    {
    this->DataObject->Update();
    }
  this->Updating = 0;

  if ( this->Input->GetMTime() > this->ExecuteTime ||
  (this->DataObject && this->DataObject->GetMTime() > this->ExecuteTime) || 
  this->GetMTime() > this->ExecuteTime )
    {
    if ( this->Input->GetDataReleased() )
      {
      this->Input->ForceUpdate();
      }
    if ( this->DataObject && this->DataObject->GetDataReleased() ) 
      {
      this->DataObject->ForceUpdate();
      }

    if ( this->StartMethod )
      {
      (*this->StartMethod)(this->StartMethodArg);
      }
    // copy topological/geometric structure from input
    ((vtkDataSet *)this->Output)->CopyStructure((vtkDataSet *)this->Input);
    // reset AbortExecute flag and Progress
    this->AbortExecute = 0;
    this->Progress = 0.0;
    this->Execute();
    this->ExecuteTime.Modified();
    if ( !this->AbortExecute )
      {
      this->UpdateProgress(1.0);
      }
    this->SetDataReleased(0);
    if ( this->EndMethod )
      {
      (*this->EndMethod)(this->EndMethodArg);
      }
    }

  if ( this->Input->ShouldIReleaseData() )
    {
    this->Input->ReleaseData();
    }
  if ( this->DataObject && this->DataObject->ShouldIReleaseData() ) 
    {
    this->DataObject->ReleaseData();
    }
}

// Merge it all together
void vtkMergeDataObjectFilter::Execute()
{
  vtkDataObject *dataObject=this->GetDataObject();
  vtkFieldData *fd=dataObject->GetFieldData();
  vtkDataSet *input=this->GetInput();
  vtkDataSet *output=this->GetOutput();
  
  vtkDebugMacro(<<"Merging dataset and data object");

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

void vtkMergeDataObjectFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  if ( this->DataObject )
    {
    os << indent << "Data Object: (" << this->DataObject << ")\n";
    }
  else
    {
    os << indent << "Data Object: (none)\n";
    }

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

