/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageImport.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-1999 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "vtkByteSwap.h"
#include "vtkImageImport.h"

//----------------------------------------------------------------------------
vtkImageImport::vtkImageImport()
{
  int idx;
  
  this->ImportVoidPointer = 0;

  this->DataScalarType = VTK_SHORT;
  this->NumberOfScalarComponents = 1;
  
  for (idx = 0; idx < 3; ++idx)
    {
    this->DataExtent[idx*2] = this->DataExtent[idx*2 + 1] = 0;
    this->DataSpacing[idx] = 1.0;
    this->DataOrigin[idx] = 0.0;
    }
  this->SaveUserArray = 0;
}

//----------------------------------------------------------------------------
vtkImageImport::~vtkImageImport()
{ 
  if ((this->ImportVoidPointer) && (!this->SaveUserArray))
    {
    delete [] this->ImportVoidPointer;
    }
}

//----------------------------------------------------------------------------
void vtkImageImport::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  vtkImageSource::PrintSelf(os,indent);

  os << indent << "ImportVoidPointer: " << this->ImportVoidPointer << "\n";

  os << indent << "DataScalarType: " 
     << vtkImageScalarTypeNameMacro(this->DataScalarType) << "\n";

  os << indent << "NumberOfScalarComponents: " 
     << this->NumberOfScalarComponents << "\n";
 
  os << indent << "DataExtent: (" << this->DataExtent[0];
  for (idx = 1; idx < 6; ++idx)
    {
    os << ", " << this->DataExtent[idx];
    }
  os << ")\n";
  
  os << indent << "DataSpacing: (" << this->DataSpacing[0];
  for (idx = 1; idx < 3; ++idx)
    {
    os << ", " << this->DataSpacing[idx];
    }
  os << ")\n";
  
  os << indent << "DataOrigin: (" << this->DataOrigin[0];
  for (idx = 1; idx < 3; ++idx)
    {
    os << ", " << this->DataOrigin[idx];
    }
  os << ")\n";
}


//----------------------------------------------------------------------------
// This method returns the largest data that can be generated.
void vtkImageImport::ExecuteInformation()
{
  // set the extent
  this->GetOutput()->SetWholeExtent(this->DataExtent);
    
  // set the spacing
  this->GetOutput()->SetSpacing(this->DataSpacing);

  // set the origin.
  this->GetOutput()->SetOrigin(this->DataOrigin);

  // set data type
  this->GetOutput()->SetScalarType(this->DataScalarType);
  this->GetOutput()->
    SetNumberOfScalarComponents(this->NumberOfScalarComponents);
}

//----------------------------------------------------------------------------
// This function reads a data from a file.  The datas extent/axes
// are assumed to be the same as the file extent/order.
void vtkImageImport::Execute(vtkImageData *data)
{
  void *ptr = this->GetImportVoidPointer();
  int size = 
    (this->DataExtent[1] - this->DataExtent[0]+1) *
    (this->DataExtent[3] - this->DataExtent[2]+1) *
    (this->DataExtent[5] - this->DataExtent[4]+1) *
    this->NumberOfScalarComponents;    
  data->GetPointData()->GetScalars()->GetData()->SetVoidArray(ptr,size,1);
}


void vtkImageImport::CopyImportVoidPointer(void *ptr, int size)
{
  unsigned char *mem = new unsigned char [size];
  memcpy(mem,ptr,size);
  this->SetImportVoidPointer(mem,0);
}

void vtkImageImport::SetImportVoidPointer(void *ptr)
{
  this->SetImportVoidPointer(ptr,1);
}

void vtkImageImport::SetImportVoidPointer(void *ptr, int save)
{
  if (ptr != this->ImportVoidPointer)
    {
    if ((this->ImportVoidPointer) && (!this->SaveUserArray))
      {
      vtkDebugMacro (<< "Deleting the array...");
      delete [] this->ImportVoidPointer;
      }
    else 
      {
      vtkDebugMacro (<<"Warning, array not deleted, but will point to new array.");
      }
    this->Modified();
    }
  this->SaveUserArray = save;
  this->ImportVoidPointer = ptr;
}


//----------------------------------------------------------------------------
void vtkImageImport::ModifyOutputUpdateExtent()
{
  int *wholeExtent, updateExtent[6], idx;
  
  this->GetOutput()->GetUpdateExtent(updateExtent);
  wholeExtent = this->GetOutput()->GetWholeExtent();
  for (idx = 0; idx < 3; ++idx)
    {
    updateExtent[idx*2] = wholeExtent[idx*2];
    updateExtent[idx*2+1] = wholeExtent[idx*2+1];
    }
  this->GetOutput()->SetUpdateExtent(updateExtent);
  this->Modified();
}
