/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderLargeImage.cxx
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
#include "vtkRenderLargeImage.h"
#include "vtkRenderWindow.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkRenderLargeImage* vtkRenderLargeImage::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkRenderLargeImage");
  if(ret)
    {
    return (vtkRenderLargeImage*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkRenderLargeImage;
}




//----------------------------------------------------------------------------
vtkRenderLargeImage::vtkRenderLargeImage()
{
  this->Input = NULL;
  this->Magnification = 3;
}

//----------------------------------------------------------------------------
vtkRenderLargeImage::~vtkRenderLargeImage()
{
  if (this->Input)
    {
    this->Input->UnRegister(this);
    this->Input = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkRenderLargeImage::PrintSelf(ostream& os, vtkIndent indent)
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

  os << indent << "Magnification: " << this->Magnification << "\n";
}


//----------------------------------------------------------------------------
// Description:
// This method returns the largest region that can be generated.
void vtkRenderLargeImage::ExecuteInformation()
{
  if (this->Input == NULL )
    {
    vtkErrorMacro(<<"Please specify a renderer as input!");
    return;
    }

  // set the extent, if the VOI has not been set then default to
  this->GetOutput()->SetWholeExtent(0, 
		       this->Magnification*
		       this->Input->GetRenderWindow()->GetSize()[0] - 1,
		       0, 
		       this->Magnification*
		       this->Input->GetRenderWindow()->GetSize()[1] - 1,
		       0, 0);

  // set the spacing
  this->GetOutput()->SetSpacing(1.0, 1.0, 1.0);

  // set the origin.
  this->GetOutput()->SetOrigin(0.0, 0.0, 0.0);
  
  // set the scalar components
  this->GetOutput()->SetNumberOfScalarComponents(3);
  this->GetOutput()->SetScalarType(VTK_UNSIGNED_CHAR);
}



//----------------------------------------------------------------------------
// Description:
// This function reads a region from a file.  The regions extent/axes
// are assumed to be the same as the file extent/order.
void vtkRenderLargeImage::ExecuteData(vtkDataObject *output)
{
  vtkImageData *data = this->AllocateOutputData(output);
  int inExtent[6];
  int inIncr[3];
  int *size;
  int inWindowExtent[4];
  double viewAngle, parallelScale, windowCenter[2];
  vtkCamera *cam;
  unsigned char *pixels, *outPtr;
  int x, y, row;
  int rowSize, rowStart, rowEnd, colStart, colEnd;
  
  if (this->GetOutput()->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
    vtkErrorMacro("mismatch in scalar types!");
    return;
    }
  
  // Get the requested extents.
  this->GetOutput()->GetUpdateExtent(inExtent);

  // get and transform the increments
  data->GetIncrements(inIncr);
  
  // get the size of the render window
  size = this->Input->GetRenderWindow()->GetSize();

  // convert the request into windows
  inWindowExtent[0] = inExtent[0]/size[0];
  inWindowExtent[1] = inExtent[1]/size[0];
  inWindowExtent[2] = inExtent[2]/size[1];
  inWindowExtent[3] = inExtent[3]/size[1];

  // store the old view angle & set the new
  cam = this->Input->GetActiveCamera();
  cam->GetWindowCenter(windowCenter);
  viewAngle = cam->GetViewAngle();
  parallelScale = cam->GetParallelScale();
  cam->SetViewAngle(asin(sin(viewAngle*3.1415926/360.0)/this->Magnification) 
		    * 360.0 / 3.1415926);
  cam->SetParallelScale(parallelScale/this->Magnification);
  
  // render each of the tiles required to fill this request
  for (y = inWindowExtent[2]; y <= inWindowExtent[3]; y++)
    {
    for (x = inWindowExtent[0]; x <= inWindowExtent[1]; x++)
      {
      cam->SetWindowCenter(x*2 - this->Magnification*(1-windowCenter[0]) + 1, 
			   y*2 - this->Magnification*(1-windowCenter[1]) + 1);
      this->Input->GetRenderWindow()->Render();
      pixels = this->Input->GetRenderWindow()->GetPixelData(0,0,size[0] - 1,
							    size[1] - 1, 1);

      // now stuff the pixels into the data row by row
      colStart = inExtent[0] - x*size[0];
      if (colStart < 0)
	{
	colStart = 0;
	}
      colEnd = size[0] - 1;
      if (colEnd > (inExtent[1] - x*size[0]))
	{
	colEnd = inExtent[1] - x*size[0];
	}
      rowSize = colEnd - colStart + 1;
	  
      // get the output pointer and do arith on it if necc
      outPtr = 
	(unsigned char *)data->GetScalarPointer(inExtent[0],inExtent[2],0);
      outPtr = outPtr + (x*size[0] - inExtent[0])*inIncr[0] + 
	(y*size[1] - inExtent[2])*inIncr[1];

      rowStart = inExtent[2] - y*size[1];
      if (rowStart < 0)
	{
	rowStart = 0;
	}
      rowEnd = size[1] - 1;
      if (rowEnd > (inExtent[3] - y*size[1]))
	{
	rowEnd = (inExtent[3] - y*size[1]);
	}
      for (row = rowStart; row <= rowEnd; row++)
	{
	memcpy(outPtr + row*inIncr[1] + colStart*inIncr[0], 
	       pixels + row*size[0]*3 + colStart*3, rowSize*3);
	}
      // free the memory
      delete [] pixels;
      }
    }

  cam->SetViewAngle(viewAngle);
  cam->SetParallelScale(parallelScale);
  cam->SetWindowCenter(windowCenter[0],windowCenter[1]);
}


