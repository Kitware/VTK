/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBMPWriter.cxx
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
#include "vtkBMPWriter.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkBMPWriter* vtkBMPWriter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkBMPWriter");
  if(ret)
    {
    return (vtkBMPWriter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkBMPWriter;
}




vtkBMPWriter::vtkBMPWriter()
{
  this->FileLowerLeft = 1;
}

void vtkBMPWriter::WriteFileHeader(ofstream *file, vtkImageData *cache)
{
  int min0, max0, min1, max1, min2, max2;
  long temp;
  int width, height, dataWidth;
  int row;
  
  // Find the length of the rows to write.
  cache->GetWholeExtent(min0, max0, min1, max1, min2, max2);
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
  for (row = 0; row < 5; row++)
    {
    file->put((char)0);
    }
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
  for (row = 0; row < 25; row++)
    {
    file->put((char)0);
    }
}


void vtkBMPWriter::WriteFile(ofstream *file, vtkImageData *data,
			     int extent[6])
{
  int idx1, idx2;
  int rowLength, rowAdder, i; // in bytes
  unsigned char *ptr;
  int bpp;
  unsigned long count = 0;
  unsigned long target;
  float progress = this->Progress;
  float area;
  int *wExtent;
  
  bpp = data->GetNumberOfScalarComponents();
  
  // Make sure we actually have data.
  if ( !data->GetPointData()->GetScalars())
    {
    vtkErrorMacro(<< "Could not get data from input.");
    return;
    }

  // take into consideration the scalar type
  if (data->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
    vtkErrorMacro("PNMWriter only accepts unsigned char scalars!");
    return; 
    }

  // Row length of x axis
  rowLength = extent[1] - extent[0] + 1;
  rowAdder = (4 - ((extent[1]-extent[0] + 1)*3)%4)%4;

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
    for (idx1 = extent[2]; idx1 <= extent[3]; idx1++)
      {
      if (!(count%target))
	{
	this->UpdateProgress(progress + count/(50.0*target));
	}
      count++;
      ptr = (unsigned char *)data->GetScalarPointer(extent[0], idx1, idx2);
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
      for (i = 0; i < rowAdder; i++)
	{
	file->put((char)0);
	}
      }
    }
}



