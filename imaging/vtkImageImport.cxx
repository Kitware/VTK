/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageImport.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

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
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "vtkByteSwap.h"
#include "vtkImageImport.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkImageImport* vtkImageImport::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageImport");
  if(ret)
    {
    return (vtkImageImport*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageImport;
}




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
    delete [] (char *)this->ImportVoidPointer;
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
  vtkImageData *output = this->GetOutput();
  
  // set the extent
  output->SetWholeExtent(this->DataExtent);
    
  // set the spacing
  output->SetSpacing(this->DataSpacing);

  // set the origin.
  output->SetOrigin(this->DataOrigin);

  // set data type
  output->SetScalarType(this->DataScalarType);
  output->SetNumberOfScalarComponents(this->NumberOfScalarComponents);
}

//----------------------------------------------------------------------------
// This function reads a data from a file.  The datas extent/axes
// are assumed to be the same as the file extent/order.
void vtkImageImport::ExecuteData(vtkDataObject *output)
{
  vtkImageData *data = this->AllocateOutputData(output);
  void *ptr = this->GetImportVoidPointer();
  int size = 
    (this->DataExtent[1] - this->DataExtent[0]+1) *
    (this->DataExtent[3] - this->DataExtent[2]+1) *
    (this->DataExtent[5] - this->DataExtent[4]+1) *
    this->NumberOfScalarComponents;    

  data->SetExtent(this->DataExtent);
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
      delete [] (char *)this->ImportVoidPointer;
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



