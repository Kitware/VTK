/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWindowToImageFilter.cxx
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
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "vtkWindowToImageFilter.h"
#include "vtkWindow.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkWindowToImageFilter* vtkWindowToImageFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkWindowToImageFilter");
  if(ret)
    {
    return (vtkWindowToImageFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkWindowToImageFilter;
}




//----------------------------------------------------------------------------
vtkWindowToImageFilter::vtkWindowToImageFilter()
{
  this->Input = NULL;
}

//----------------------------------------------------------------------------
vtkWindowToImageFilter::~vtkWindowToImageFilter()
{
  if (this->Input)
    {
    this->Input->UnRegister(this);
    this->Input = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkWindowToImageFilter::SetInput(vtkWindow *input)
{
  if (input != this->Input)
    {
    if (this->Input) {this->Input->UnRegister(this);}
    this->Input = input;
    if (this->Input) {this->Input->Register(this);}
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkWindowToImageFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageSource::PrintSelf(os,indent);
  
  if ( this->Input )
    {
    os << indent << "Input:\n";
    this->Input->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Input: (none)\n";
    }
}


//----------------------------------------------------------------------------
// This method returns the largest region that can be generated.
void vtkWindowToImageFilter::ExecuteInformation()
{
  if (this->Input == NULL )
    {
    vtkErrorMacro(<<"Please specify a renderer as input!");
    return;
    }
  vtkImageData *out = this->GetOutput();
  
  // set the extent, if the VOI has not been set then default to
  out->SetWholeExtent(0, this->Input->GetSize()[0] - 1,
		      0, this->Input->GetSize()[1] - 1,
		      0, 0);
  
  // set the spacing
  out->SetSpacing(1.0, 1.0, 1.0);
  
  // set the origin.
  out->SetOrigin(0.0, 0.0, 0.0);
  
  // set the scalar components
  out->SetNumberOfScalarComponents(3);
  out->SetScalarType(VTK_UNSIGNED_CHAR);
}



//----------------------------------------------------------------------------
// This function reads a region from a file.  The regions extent/axes
// are assumed to be the same as the file extent/order.
void vtkWindowToImageFilter::ExecuteData(vtkDataObject *vtkNotUsed(data))
{
  vtkImageData *out = this->GetOutput();
  out->SetExtent(out->GetUpdateExtent());
  out->AllocateScalars();
  
  int outExtent[6];
  int outIncr[3];
  int *size;
  unsigned char *pixels, *outPtr, *pixels1;
  int idxY, rowSize;
  
  if (out->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
    vtkErrorMacro("mismatch in scalar types!");
    return;
    }
  
  // Get the requested extents.
  out->GetUpdateExtent(outExtent);
  out->GetIncrements(outIncr);
  rowSize = (outExtent[1] - outExtent[0] + 1)*3;
  
  // get the size of the render window
  size = this->Input->GetSize();

  pixels = this->Input->GetPixelData(0,0,size[0] - 1, size[1] - 1, 1); 
  pixels1 = pixels;
  
  outPtr = 
    (unsigned char *)out->GetScalarPointer(outExtent[0], outExtent[2],0);
  
  // Loop through ouput pixels
  for (idxY = outExtent[2]; idxY <= outExtent[3]; idxY++)
    {
    memcpy(outPtr,pixels1,rowSize);
    outPtr += outIncr[1];
    pixels1 = pixels1 + size[0]*3;
    }
  
  // free the memory
  delete [] pixels;
}



