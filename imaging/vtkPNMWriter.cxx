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
  int min1, max1, min2, max2, min3, max3;
  int bpp;
  
  // Find the length of the rows to write.
  cache->GetWholeExtent(min1, max1, min2, max2, min3, max3);
  bpp = cache->GetNumberOfScalarComponents();
  
  // spit out the pnm header
  if (bpp == 1)
    {
    *file << "P5\n";
    *file << "# pgm file written by the visualization toolkit\n";
    *file << (max1 - min1 + 1) << " " << (max2 - min2 + 1) << "\n255\n";
    }
  else 
    {
    *file << "P6\n";
    *file << "# ppm file written by the visualization toolkit\n";
    *file << (max1 - min1 + 1) << " " << (max2 - min2 + 1) << "\n255\n";
    }
}


void vtkPNMWriter::WriteFile(ofstream *file, vtkImageData *data,
			     int extent[6])
{
  int idx0, idx1, idx2;
  int rowLength; // in bytes
  void *ptr;
  unsigned long count = 0;
  unsigned long target;
  float progress = this->Progress;
  float area;
  int *wExtent;
  
  // Make sure we actually have data.
  if ( !data->GetPointData()->GetScalars())
    {
    vtkErrorMacro(<< "Could not get data from input.");
    return;
    }

  // take into consideration the scalar type
  switch (data->GetScalarType())
    {
    case VTK_UNSIGNED_CHAR:
      rowLength = sizeof(unsigned char); 
      break;
    default:
      vtkErrorMacro("PNMWriter only accepts unsigned char scalars!");
      return; 
    }
  rowLength *= data->GetNumberOfScalarComponents();

  wExtent = this->Input->GetWholeExtent();
  area = ((extent[5] - extent[4] + 1)*(extent[3] - extent[2] + 1)*
	  (extent[1] - extent[0] + 1)) / 
    ((wExtent[5] -wExtent[4] + 1)*(wExtent[3] -wExtent[2] + 1)*
     (wExtent[1] -wExtent[0] + 1));
    
  target = (unsigned long)((extent[5]-extent[4]+1)*
			   (extent[3]-extent[2]+1)/(50.0*area));
  target++;

  for (idx2 = extent[4]; idx2 <= extent[5]; ++idx2)
    {
    for (idx1 = extent[3]; idx1 >= extent[2]; idx1--)
      {
      if (!(count%target))
	{
	this->UpdateProgress(progress + count/(50.0*target));
	}
      count++;
      for (idx0 = extent[0]; idx0 <= extent[1]; idx0++)
	{
	ptr = data->GetScalarPointer(idx0, idx1, idx2);
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



