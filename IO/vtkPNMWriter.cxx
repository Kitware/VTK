/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPNMWriter.cxx
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
#include "vtkPNMWriter.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkPNMWriter* vtkPNMWriter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPNMWriter");
  if(ret)
    {
    return (vtkPNMWriter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPNMWriter;
}




#ifdef write
#undef write
#endif

#ifdef close
#undef close
#endif

void vtkPNMWriter::WriteFileHeader(ofstream *file, vtkImageData *cache)
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

  wExtent = this->GetInput()->GetWholeExtent();
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



