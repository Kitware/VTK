/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPNMWriter.cxx
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
#include "vtkPNMWriter.h"

void vtkPNMWriter::WriteFileHeader(ofstream *file, vtkImageCache *cache)
{
  int min0, max0, min1, max1, min2, max2, min3, max3, minC, maxC;
  int bpp;
  
  // Find the length of the rows to write.
  cache->GetUpdateExtent(min0, max0, min1, max1, min2, max2, min3, max3);
  cache->GetAxisUpdateExtent(VTK_IMAGE_COMPONENT_AXIS, minC, maxC);
  bpp = maxC - minC + 1;
  
  // spit out the pnm header
  if (bpp == 1)
    {
    *file << "P5\n";
    *file << "# pgm file written by the visualization toolkit\n";
    *file << (max0 - min0 + 1) << " " << (max1 - min1 + 1) << "\n255\n";
    }
  else 
    {
    *file << "P6\n";
    *file << "# ppm file written by the visualization toolkit\n";
    *file << (max0 - min0 + 1) << " " << (max1 - min1 + 1) << "\n255\n";
    }
}


void vtkPNMWriter::WriteFile(ofstream *file, vtkImageRegion *region)
{
  int min0, max0, min1, max1, min2, max2, min3, max3, minC, maxC;
  int idx1, idx2, idx3;
  int rowLength; // in bytes
  void *ptr;
  int bpp;
  vtkImageRegion *data;
  
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
  rowLength = bpp*(max0 - min0 + 1);

  for (idx3 = min3; idx3 <= max3; ++idx3)
    {
    for (idx2 = min2; idx2 <= max2; ++idx2)
      {
      for (idx1 = max1; idx1 >= min1; idx1--)
	{
	ptr = data->GetScalarPointer(min0, idx1, idx2, idx3);
	if ( ! file->write((char *)ptr, rowLength))
	  {
	  vtkErrorMacro("WriteFile: write failed");
	  file->close();
	  delete file;
	  }
	}
      }
    }
}



