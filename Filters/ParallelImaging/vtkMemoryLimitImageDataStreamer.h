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
/**
 * @class   vtkMemoryLimitImageDataStreamer
 * @brief   Initiates streaming on image data.
 *
 * To satisfy a request, this filter calls update on its input
 * many times with smaller update extents.  All processing up stream
 * streams smaller pieces.
*/

#ifndef vtkMemoryLimitImageDataStreamer_h
#define vtkMemoryLimitImageDataStreamer_h

#include "vtkFiltersParallelImagingModule.h" // For export macro
#include "vtkImageDataStreamer.h"


class VTKFILTERSPARALLELIMAGING_EXPORT vtkMemoryLimitImageDataStreamer : public vtkImageDataStreamer
{
public:
  static vtkMemoryLimitImageDataStreamer *New();
  vtkTypeMacro(vtkMemoryLimitImageDataStreamer,vtkImageDataStreamer);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Set / Get the memory limit in kibibytes (1024 bytes).
   */
  vtkSetMacro(MemoryLimit, unsigned long);
  vtkGetMacro(MemoryLimit, unsigned long);
  //@}

  // See the vtkAlgorithm for a desciption of what these do
  int ProcessRequest(vtkInformation*,
                     vtkInformationVector**,
                     vtkInformationVector*);

protected:
  vtkMemoryLimitImageDataStreamer();
  ~vtkMemoryLimitImageDataStreamer() {}

  unsigned long  MemoryLimit;
private:
  vtkMemoryLimitImageDataStreamer(const vtkMemoryLimitImageDataStreamer&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMemoryLimitImageDataStreamer&) VTK_DELETE_FUNCTION;
};




#endif
