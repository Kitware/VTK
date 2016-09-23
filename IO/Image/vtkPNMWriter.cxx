/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPNMWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPNMWriter.h"

#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkPNMWriter);

#ifdef write
#undef write
#endif

#ifdef close
#undef close
#endif

void vtkPNMWriter::WriteFileHeader(ofstream *file,
                                   vtkImageData *cache,
                                   int wExt[6])
{
  int min1 = wExt[0],
    max1 = wExt[1],
    min2 = wExt[2],
    max2 = wExt[3];
  int bpp;

  // Find the length of the rows to write.
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
                             int extent[6], int wExtent[6])
{
  int idx0, idx1, idx2;
  int rowLength; // in bytes
  void *ptr;
  unsigned long count = 0;
  unsigned long target;
  float progress = this->Progress;
  float area;

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

  area = static_cast<float>(((extent[5] - extent[4] + 1)*(extent[3] - extent[2] + 1)*
                             (extent[1] - extent[0] + 1))) /
         static_cast<float>(((wExtent[5] -wExtent[4] + 1)*(wExtent[3] -wExtent[2] + 1)*
                             (wExtent[1] -wExtent[0] + 1)));

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
          this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
          return;
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkPNMWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
