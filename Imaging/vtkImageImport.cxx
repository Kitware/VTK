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
    this->WholeExtent[idx*2] = this->WholeExtent[idx*2 + 1] = 0;
    this->DataSpacing[idx] = 1.0;
    this->DataOrigin[idx] = 0.0;
    }
  this->SaveUserArray = 0;
  
  this->CallbackUserData = 0;

  this->UpdateInformationCallback = 0;
  this->PipelineModifiedCallback = 0;
  this->WholeExtentCallback = 0;
  this->SpacingCallback = 0;
  this->OriginCallback = 0;
  this->ScalarTypeCallback = 0;
  this->NumberOfComponentsCallback = 0;
  this->PropagateUpdateExtentCallback = 0;
  this->UpdateDataCallback = 0;
  this->DataExtentCallback = 0;
  this->BufferPointerCallback = 0;
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
 
  os << indent << "WholeExtent: (" << this->WholeExtent[0];
  for (idx = 1; idx < 6; ++idx)
    {
    os << ", " << this->WholeExtent[idx];
    }
  os << ")\n";
  
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

  os << indent << "CallbackUserData: "
     << (this->CallbackUserData? "Set" : "Not Set") << "\n";
  
  os << indent << "UpdateInformationCallback: "
     << (this->UpdateInformationCallback? "Set" : "Not Set") << "\n";
  
  os << indent << "PipelineModifiedCallback: "
     << (this->PipelineModifiedCallback? "Set" : "Not Set") << "\n";
  
  os << indent << "WholeExtentCallback: "
     << (this->WholeExtentCallback? "Set" : "Not Set") << "\n";
  
  os << indent << "SpacingCallback: "
     << (this->SpacingCallback? "Set" : "Not Set") << "\n";
  
  os << indent << "OriginCallback: "
     << (this->OriginCallback? "Set" : "Not Set") << "\n";
  
  os << indent << "ScalarTypeCallback: "
     << (this->ScalarTypeCallback? "Set" : "Not Set") << "\n";
  
  os << indent << "NumberOfComponentsCallback: "
     << (this->NumberOfComponentsCallback? "Set" : "Not Set") << "\n";
  
  os << indent << "PropagateUpdateExtentCallback: "
     << (this->PropagateUpdateExtentCallback? "Set" : "Not Set") << "\n";
  
  os << indent << "UpdateDataCallback: "
     << (this->UpdateDataCallback? "Set" : "Not Set") << "\n";
  
  os << indent << "DataExtentCallback: "
     << (this->DataExtentCallback? "Set" : "Not Set") << "\n";
  
  os << indent << "BufferPointerCallback: "
     << (this->BufferPointerCallback? "Set" : "Not Set") << "\n";
}

//----------------------------------------------------------------------------
void vtkImageImport::PropagateUpdateExtent(vtkDataObject *output)
{
  vtkImageSource::PropagateUpdateExtent(output);
  if (this->PropagateUpdateExtentCallback)
    {
    (this->PropagateUpdateExtentCallback)(this->CallbackUserData,
                                          output->GetUpdateExtent());
    }
}

//----------------------------------------------------------------------------
void vtkImageImport::UpdateInformation()
{
  // If set, use the callbacks to propagate pipeline update information.
  this->InvokeUpdateInformationCallbacks();
  
  vtkImageSource::UpdateInformation();
}

//----------------------------------------------------------------------------
void vtkImageImport::ExecuteInformation()
{
  // If set, use the callbacks to fill in our data members.
  this->InvokeExecuteInformationCallbacks();
  
  // Legacy support for code that sets only DataExtent.
  this->LegacyCheckWholeExtent();
  
  vtkImageData *output = this->GetOutput();
  
  // set the whole extent
  output->SetWholeExtent(this->WholeExtent);
  
  // set the spacing
  output->SetSpacing(this->DataSpacing);

  // set the origin.
  output->SetOrigin(this->DataOrigin);

  // set data type
  output->SetScalarType(this->DataScalarType);
  output->SetNumberOfScalarComponents(this->NumberOfScalarComponents);
}

//----------------------------------------------------------------------------
void vtkImageImport::ExecuteData(vtkDataObject *output)
{
  // If set, use the callbacks to prepare our input data.
  this->InvokeExecuteDataCallbacks();
  
  vtkImageData *data = this->AllocateOutputData(output);
  void *ptr = this->GetImportVoidPointer();
  int size = 
    (this->DataExtent[1] - this->DataExtent[0]+1) *
    (this->DataExtent[3] - this->DataExtent[2]+1) *
    (this->DataExtent[5] - this->DataExtent[4]+1) *
    this->NumberOfScalarComponents;    

  data->SetExtent(this->DataExtent);
  data->GetPointData()->GetScalars()->SetVoidArray(ptr,size,1);
}

//----------------------------------------------------------------------------
void vtkImageImport::CopyImportVoidPointer(void *ptr, int size)
{
  unsigned char *mem = new unsigned char [size];
  memcpy(mem,ptr,size);
  this->SetImportVoidPointer(mem,0);
}

//----------------------------------------------------------------------------
void vtkImageImport::SetImportVoidPointer(void *ptr)
{
  this->SetImportVoidPointer(ptr,1);
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkImageImport::InvokeUpdateInformationCallbacks()
{
  if (this->UpdateInformationCallback)
    {
    (this->UpdateInformationCallback)(this->CallbackUserData);
    }
  if (this->PipelineModifiedCallback)
    {
    if ((this->PipelineModifiedCallback)(this->CallbackUserData))
      {
      this->Modified();
      }
    }  
}

//----------------------------------------------------------------------------
void vtkImageImport::InvokeExecuteInformationCallbacks()
{
  if (this->WholeExtentCallback)
    {
    this->SetWholeExtent((this->WholeExtentCallback)(this->CallbackUserData));
    }
  if (this->SpacingCallback)
    {
    this->SetDataSpacing((this->SpacingCallback)(this->CallbackUserData));
    }
  if (this->OriginCallback)
    {
    this->SetDataOrigin((this->OriginCallback)(this->CallbackUserData));
    }
  if (this->NumberOfComponentsCallback)
    {
    this->SetNumberOfScalarComponents(
      (this->NumberOfComponentsCallback)(this->CallbackUserData));
    }
  if (this->ScalarTypeCallback)
    {
    const char* scalarType =
      (this->ScalarTypeCallback)(this->CallbackUserData);
    if (strcmp(scalarType, "double")==0)
      {
      this->SetDataScalarType(VTK_DOUBLE);
      }
    else if (strcmp(scalarType, "float")==0)
      {
      this->SetDataScalarType(VTK_FLOAT);
      }
    else if (strcmp(scalarType, "long")==0)
      {
      this->SetDataScalarType(VTK_LONG);
      }
    else if (strcmp(scalarType, "unsigned long")==0)
      {
      this->SetDataScalarType(VTK_UNSIGNED_LONG);
      }
    else if (strcmp(scalarType, "int")==0)
      {
      this->SetDataScalarType(VTK_INT);
      }
    else if (strcmp(scalarType, "unsigned int")==0)
      {
      this->SetDataScalarType(VTK_UNSIGNED_INT);
      }
    else if (strcmp(scalarType, "short")==0)
      {
      this->SetDataScalarType(VTK_SHORT);
      }
    else if (strcmp(scalarType, "unsigned short")==0)
      {
      this->SetDataScalarType(VTK_UNSIGNED_SHORT);
      }
    else if (strcmp(scalarType, "char")==0)
      {
      this->SetDataScalarType(VTK_CHAR);
      }
    else if (strcmp(scalarType, "unsigned char")==0)
      {
      this->SetDataScalarType(VTK_UNSIGNED_CHAR);
      }    
    }
}


//----------------------------------------------------------------------------
void vtkImageImport::InvokeExecuteDataCallbacks()
{
  if (this->UpdateDataCallback)
    {
    (this->UpdateDataCallback)(this->CallbackUserData);
    }
  if (this->DataExtentCallback)
    {
    this->SetDataExtent((this->DataExtentCallback)(this->CallbackUserData));
    }
  if (this->BufferPointerCallback)
    {
    this->SetImportVoidPointer(
      (this->BufferPointerCallback)(this->CallbackUserData));
    }
}  

//----------------------------------------------------------------------------
// In the past, this class made no distinction between whole extent and
// buffered extent, so only SetDataExtent also set the whole extent of
// the output.  Now, there is a separate SetWholeExtent which should be
// called as well.
void vtkImageImport::LegacyCheckWholeExtent()
{
  // If the WholeExtentCallback is set, this must not be legacy code.
  if (this->WholeExtentCallback)
    {
    return;
    }
  int i;
  // Check whether the whole extent has been set.
  for(i=0; i < 6; ++i)
    {
    if (this->WholeExtent[i] != 0)
      {
      return;
      }
    }
  
  // The whole extent has not been set.  Copy it from the data extent
  // and issue a warning.
  for(i=0; i < 6; ++i)
    {
    this->WholeExtent[i] = this->DataExtent[i];
    }
  
  vtkWarningMacro("\n"
    "There is a distinction between the whole extent and the buffered\n"
    "extent of an imported image.  Use SetWholeExtent to set the extent\n"
    "of the entire image.  Use SetDataExtent to set the extent of the\n"
    "portion of the image that is in the buffer set with\n"
    "SetImportVoidPointer.  Both should be called even if the extents are\n"
    "the same.");
}
