/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageImport.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder,ill Lorensen.

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
#include "vtkImageImport.h"

//----------------------------------------------------------------------------
vtkImageImport::vtkImageImport()
{
  this->Region = vtkImageRegion::New();
  this->Pointer = NULL;
  this->Pointers = NULL;
  this->Initialized = 0;
}

//----------------------------------------------------------------------------
vtkImageImport::~vtkImageImport()
{
  this->Region->Delete();
}

//----------------------------------------------------------------------------
void vtkImageImport::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageSource::PrintSelf(os,indent);
  os << indent << "DataScalarType: " 
     << vtkImageScalarTypeNameMacro(this->DataScalarType) << "\n";
  os << indent << "Region: (" << this->Region << ")\n";
  this->Region->PrintSelf(os,indent.GetNextIndent());
  os << indent << "Pointer: (" << this->Pointer << ")\n";
  os << indent << "Pointers: (" << this->Pointers << ")\n";
  os << indent << "Initialized: " << this->Initialized << "\n";
}



//----------------------------------------------------------------------------
void vtkImageImport::SetDataAxes(int dim, int *axes)
{
  this->Region->ReleaseData();
  this->Region->SetAxes(dim, axes);
  // Keep a local copy for macros
  this->Region->GetAxes(VTK_IMAGE_DIMENSIONS, this->DataAxes);
}

//----------------------------------------------------------------------------
void vtkImageImport::SetDataExtent(int dim, int *extent)
{
  this->Region->ReleaseData();
  this->Region->SetExtent(dim, extent);
  // Keep a local copy for macros
  this->Region->GetExtent(VTK_IMAGE_DIMENSIONS, this->DataExtent);
}

//----------------------------------------------------------------------------
void vtkImageImport::SetPointer(void *ptr)
{
  this->Initialized = 0;
  this->Pointers = NULL;
  this->Pointer = ptr;
}

//----------------------------------------------------------------------------
void vtkImageImport::SetPointers(void **ptrs)
{
  this->Initialized = 0;
  this->Pointers = ptrs;
  this->Pointer = NULL;
}


//----------------------------------------------------------------------------
void vtkImageImport::InitializeRegion()
{
  int length;
  void *regionPtr;
  vtkImageData *data;
  
  if ( this->Initialized)
    {
    return;
    }
  if (this->Pointer)
    {
    this->Region->ReleaseData();
    // Compute the length of the memory segment.
    length = this->Region->GetExtentMemorySize();
    
    // make sure the underlying data object is set correctly
    data = vtkImageData::New();
    data->SetScalarType(this->DataScalarType);
    data->SetAxes(VTK_IMAGE_DIMENSIONS, this->DataAxes);
    data->SetExtent(VTK_IMAGE_DIMENSIONS, this->DataExtent);
    
    this->Region->SetScalarType(this->DataScalarType);
    this->Region->SetAxes(VTK_IMAGE_DIMENSIONS, this->DataAxes);
    this->Region->SetExtent(VTK_IMAGE_DIMENSIONS, this->DataExtent);
    this->Region->SetData(data);
    
    // copy the memory
    vtkDebugMacro("SetPointer: Copying " << length << " bytes");
    regionPtr = this->Region->GetScalarPointer();
    memcpy(regionPtr, this->Pointer, length);
    
    this->Pointer = NULL;
    this->Initialized = 1;
    }
  
  vtkErrorMacro("Initialize: Cannot initialize");
}

//----------------------------------------------------------------------------
// Initialize and pass the request to the region/
void vtkImageImport::Update(vtkImageRegion *region)
{
  this->InitializeRegion();
  this->Region->Update(region);
}


//----------------------------------------------------------------------------
// Just pass the request for information to the region.
void vtkImageImport::UpdateImageInformation(vtkImageRegion *region)
{
  this->Region->UpdateImageInformation(region);
}

