/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBMPWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBMPWriter.h"

#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkBMPWriter);

vtkBMPWriter::vtkBMPWriter()
{
  this->FileLowerLeft = 1;
}

void vtkBMPWriter::WriteFileHeader(ofstream *file,
                                   vtkImageData *,
                                   int wExt[6])
{
  long temp;
  int width, height, dataWidth;
  int row;

  // Find the length of the rows to write.
  width = (wExt[1] - wExt[0] + 1);
  height = (wExt[3] - wExt[2] + 1);

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
                             int extent[6], int wExtent[6])
{
  int idx1, idx2;
  int rowLength, rowAdder, i; // in bytes
  unsigned char *ptr;
  int bpp;
  unsigned long count = 0;
  unsigned long target;
  float progress = this->Progress;
  float area;

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
    vtkErrorMacro("BMPWriter only accepts unsigned char scalars!");
    return;
  }

  // Row length of x axis
  rowLength = extent[1] - extent[0] + 1;
  rowAdder = (4 - ((extent[1]-extent[0] + 1)*3)%4)%4;

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

//----------------------------------------------------------------------------
void vtkBMPWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
