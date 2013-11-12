/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMemoryLimitImageDataStreamer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMemoryLimitImageDataStreamer - Initiates streaming on image data.
// .SECTION Description
// To satisfy a request, this filter calls update on its input
// many times with smaller update extents.  All processing up stream
// streams smaller pieces.

#ifndef __vtkMemoryLimitImageDataStreamer_h
#define __vtkMemoryLimitImageDataStreamer_h

#include "vtkFiltersParallelImagingModule.h" // For export macro
#include "vtkImageDataStreamer.h"


class VTKFILTERSPARALLELIMAGING_EXPORT vtkMemoryLimitImageDataStreamer : public vtkImageDataStreamer
{
public:
  static vtkMemoryLimitImageDataStreamer *New();
  vtkTypeMacro(vtkMemoryLimitImageDataStreamer,vtkImageDataStreamer);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / Get the memory limit in kilobytes.
  vtkSetMacro(MemoryLimit, unsigned long);
  vtkGetMacro(MemoryLimit, unsigned long);

  // See the vtkAlgorithm for a desciption of what these do
  int ProcessRequest(vtkInformation*,
                     vtkInformationVector**,
                     vtkInformationVector*);

protected:
  vtkMemoryLimitImageDataStreamer();
  ~vtkMemoryLimitImageDataStreamer() {}

  unsigned long  MemoryLimit;
private:
  vtkMemoryLimitImageDataStreamer(const vtkMemoryLimitImageDataStreamer&);  // Not implemented.
  void operator=(const vtkMemoryLimitImageDataStreamer&);  // Not implemented.
};




#endif
