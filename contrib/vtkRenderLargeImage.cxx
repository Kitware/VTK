/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderLargeImage.cxx
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
#include "vtkRenderLargeImage.h"
#include "vtkRenderWindow.h"

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
void vtkRenderLargeImage::UpdateImageInformation()
{
  if (this->Input == NULL )
    {
    vtkErrorMacro(<<"Please specify a renderer as input!");
    return;
    }

  // Make sure we have an output.
  this->CheckCache();
    
  // set the extent, if the VOI has not been set then default to
  this->Output->SetWholeExtent(0, 
			       this->Magnification*
			       this->Input->GetRenderWindow()->GetSize()[0] - 1,
			       0, 
			       this->Magnification*
			       this->Input->GetRenderWindow()->GetSize()[1] - 1,
			       0, 0);

  // set the spacing
  this->Output->SetSpacing(1.0, 1.0, 1.0);

  // set the origin.
  this->Output->SetOrigin(0.0, 0.0, 0.0);
  
  // set the scalar components
  this->Output->SetNumberOfScalarComponents(3);
  this->Output->SetScalarType(VTK_UNSIGNED_CHAR);
}



//----------------------------------------------------------------------------
// Description:
// This function reads a region from a file.  The regions extent/axes
// are assumed to be the same as the file extent/order.
void vtkRenderLargeImage::Execute(vtkImageData *data)
{
  int inExtent[6];
  int inIncr[3];
  int *size;
  int inWindowExtent[4];
  float viewAngle;
  vtkCamera *cam;
  unsigned char *pixels, *outPtr;
  int x, y, row;
  int rowSize, rowStart, rowEnd, colStart, colEnd;
  
  if (this->Output->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
    vtkErrorMacro("mismatch in scalar types!");
    return;
    }
  
  // Get the requested extents.
  this->Output->GetUpdateExtent(inExtent);
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
  viewAngle = cam->GetViewAngle();
  cam->SetViewAngle(asin(sin(viewAngle*3.1415926/360.0)/this->Magnification) 
		    * 360.0 / 3.1415926);
  
  // render each of the tiles required to fill this request
  for (y = inWindowExtent[2]; y <= inWindowExtent[3]; y++)
    {
    for (x = inWindowExtent[0]; x <= inWindowExtent[1]; x++)
      {
      cam->SetWindowCenter(x*2 - this->Magnification + 1, 
			   y*2 - this->Magnification + 1);
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
  cam->SetWindowCenter(0.0,0.0);
}


