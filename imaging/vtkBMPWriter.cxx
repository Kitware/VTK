/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBMPWriter.cxx
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
#include "vtkBMPWriter.h"

vtkBMPWriter::vtkBMPWriter()
{
  this->FileLowerLeft = 1;
}

void vtkBMPWriter::WriteFileHeader(ofstream *file, vtkImageCache *cache)
{
  int min0, max0, min1, max1, min2, max2, min3, max3;
  int bpp;
  long temp;
  int width, height, dataWidth;
  int row;
  
  // Find the length of the rows to write.
  cache->GetUpdateExtent(min0, max0, min1, max1, min2, max2, min3, max3);
  bpp = cache->GetNumberOfScalarComponents();
  width = (max0 - min0 + 1);
  height = (max1 - min1 + 1);
  
  dataWidth = ((width*3+3)/4)*4;

  // spit out the BMP header
  file->put((char)66);
  file->put((char)77);
  temp = (long)(dataWidth*height) + 54L;
  file->put((char)(temp%256));
  file->put((char)((temp%65536L)/256));
  file->put((char)(temp/65536L));
  for (row = 0; row < 5; row++) file->put((char)0);
  file->put((char)54);
  file->put((char)0);
  file->put((char)0);
  file->put((char)0);
  
  // info header
  file->put((char)40);
  file->put((char)0);
  file->put((char)0);
  file->put((char)0);
  
  file->put((char)(width%256));
  file->put((char)(width/256));
  file->put((char)0);
  file->put((char)0);
  
  file->put((char)(height%256));
  file->put((char)(height/256));
  file->put((char)0);
  file->put((char)0);
  
  file->put((char)1);
  file->put((char)0);
  file->put((char)24);
  for (row = 0; row < 25; row++) file->put((char)0);
}


void vtkBMPWriter::WriteFile(ofstream *file, vtkImageRegion *region)
{
  int min0, max0, min1, max1, min2, max2, min3, max3, minC, maxC;
  int idx1, idx2, idx3;
  int rowLength; // in bytes
  char *ptr;
  int bpp;
  vtkImageRegion *data;
  int i, rowAdder;
  
  // Make sure we actually have data.
  if ( ! region->AreScalarsAllocated())
    {
    vtkErrorMacro(<< "Could not get region from input.");
    return;
    }

  // Find the length of the rows to write.
  region->GetExtent(min0, max0, min1, max1, min2, max2, min3, max3);
  region->GetData()->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, minC, maxC);
  bpp = maxC - minC + 1;
  
  // take into consideration the scalar type
  switch (region->GetScalarType())
    {
    case VTK_UNSIGNED_CHAR:
      data = region;
      break;
    case VTK_FLOAT:
    case VTK_INT:
    case VTK_SHORT:
    case VTK_UNSIGNED_SHORT:
    default:
      data = vtkImageRegion::New();
      data->SetScalarType(VTK_UNSIGNED_CHAR);
      data->SetAxes(VTK_IMAGE_COMPONENT_AXIS, VTK_IMAGE_X_AXIS, 
		    VTK_IMAGE_Y_AXIS, VTK_IMAGE_Z_AXIS);
      region->SetAxes(VTK_IMAGE_COMPONENT_AXIS, VTK_IMAGE_X_AXIS, 
		    VTK_IMAGE_Y_AXIS, VTK_IMAGE_Z_AXIS);
      data->SetExtent(4, region->GetExtent());
      data->CopyRegionData(region);
    }
  // Always write all the components
  // Row length of x axis
  rowLength = max0 - min0 + 1;
  rowAdder = (4 - ((max0-min0 + 1)*3)%4)%4;

  for (idx3 = min3; idx3 <= max3; ++idx3)
    {
    for (idx2 = min2; idx2 <= max2; ++idx2)
      {
      for (idx1 = min1; idx1 <= max1; idx1++)
	{
	ptr = (char *)data->GetScalarPointer(min0, idx1, idx2, idx3);
	if (bpp == 1)
	  {
	  for (i = 0; i < rowLength; i++)
	    {
	    file->put(ptr[i]);
	    file->put(ptr[i]);
	    file->put(ptr[i]);
	    }
	  }
	if (bpp == 2)
	  {
	  for (i = 0; i < rowLength; i++)
	    {
	    file->put(ptr[i*2]);
	    file->put(ptr[i*2]);
	    file->put(ptr[i*2]);
	    }
	  }
	if (bpp == 3)
	  {
	  for (i = 0; i < rowLength; i++)
	    {
	    file->put(ptr[i*3 + 2]);
	    file->put(ptr[i*3 + 1]);
	    file->put(ptr[i*3]);
	    }
	  }
	if (bpp == 4)
	  {
	  for (i = 0; i < rowLength; i++)
	    {
	    file->put(ptr[i*4 + 2]);
	    file->put(ptr[i*4 + 1]);
	    file->put(ptr[i*4]);
	    }
	  }
	for (i = 0; i < rowAdder; i++) file->put((char)0);
	}
      }
    }
}



