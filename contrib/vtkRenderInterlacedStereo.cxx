/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderInterlacedStereo.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "vtkImageCache.h"
#include "vtkRenderInterlacedStereo.h"
#include "vtkRenderWindow.h"

//----------------------------------------------------------------------------
vtkRenderInterlacedStereo::vtkRenderInterlacedStereo()
{
  this->Input = NULL;
}

//----------------------------------------------------------------------------
void vtkRenderInterlacedStereo::PrintSelf(ostream& os, vtkIndent indent)
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
// Description:
// This method returns the largest region that can be generated.
void vtkRenderInterlacedStereo::UpdateImageInformation()
{
  if (this->Input == NULL )
    {
    vtkErrorMacro(<<"Please specify a renderer as input!");
    return;
    }

  // Make sure we have an output.
  this->CheckCache();
    
  // set the extent
  this->Output->SetWholeExtent(0, this->Input->GetSize()[0] - 1,
			       0, this->Input->GetSize()[1] - 1,
			       0, 0);

  // set the spacing
  this->Output->SetSpacing(1.0, 1.0, 1.0);

  // set the origin.
  this->Output->SetOrigin(0.0, 0.0, 0.0);
  
  // set the scalar components (RGB)
  this->Output->SetNumberOfScalarComponents(3);
  this->Output->SetScalarType(VTK_UNSIGNED_CHAR);
}



//----------------------------------------------------------------------------
// Description:
// This function reads a region from a file.  The regions extent/axes
// are assumed to be the same as the file extent/order.
void vtkRenderInterlacedStereo::Execute(vtkImageData *data)
{
  int extent[6];
  int incs[3];
  int *size;
  unsigned char *pixels, *outPtr;
  int row;
  
  if (this->Output->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
    vtkErrorMacro("mismatch in scalar types!");
    return;
    }
  
  // Get the requested extents.
  this->Output->GetUpdateExtent(extent);
  // get and transform the increments
  data->GetIncrements(incs);
  // get the size of the render window
  size = this->Input->GetSize();
  
  // render left image
  this->Input->SetStereoTypeToRight();
  this->Input->Render();
  // Get the renderer image
  pixels = this->Input->GetPixelData(0,0,size[0] - 1, size[1] - 1, 1);
  // now stuff every other row of the pixels into the data.
  // get the output pointer
  outPtr = (unsigned char *)data->GetScalarPointer(extent[0],extent[2],0);
  for (row = extent[2]; row <= extent[3]; row += 2)
    {
    memcpy(outPtr, pixels + row*size[0]*3, size[0]*3);
    outPtr += incs[1] * 2;
    }

  // render right image
  this->Input->SetStereoTypeToLeft();
  this->Input->Render();
  // Get the renderer image
  pixels = this->Input->GetPixelData(0,0,size[0] - 1, size[1] - 1, 1);
  // now stuff every other row of the pixels into the data.
  // get the output pointer
  outPtr= (unsigned char *)data->GetScalarPointer(extent[0],extent[2]+1,0);
  for (row = extent[2]+1; row <= extent[3]; row += 2)
    {
    memcpy(outPtr, pixels + row*size[0]*3, size[0]*3);
    outPtr += incs[1] * 2;
    }

  // free the memory
  delete [] pixels;
}


