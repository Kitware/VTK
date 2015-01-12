/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransmitImageDataPiece.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTransmitRectilinearGridPiece - Redistributes data produced
// by serial readers
//
// .SECTION Description
// This filter can be used to redistribute data from producers that can't
// produce data in parallel. All data is produced on first process and
// the distributed to others using the multiprocess controller.
//
// Note that this class is legacy. The superclass does all the work and
// can be used directly instead.


#ifndef vtkTransmitImageDataPiece_h
#define vtkTransmitImageDataPiece_h

#include "vtkFiltersParallelImagingModule.h" // For export macro
#include "vtkTransmitStructuredDataPiece.h"

class vtkMultiProcessController;

class VTKFILTERSPARALLELIMAGING_EXPORT vtkTransmitImageDataPiece : public vtkTransmitStructuredDataPiece
{
public:
  static vtkTransmitImageDataPiece *New();
  vtkTypeMacro(vtkTransmitImageDataPiece, vtkTransmitStructuredDataPiece);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkTransmitImageDataPiece();
  ~vtkTransmitImageDataPiece();

private:
  vtkTransmitImageDataPiece(const vtkTransmitImageDataPiece&); // Not implemented
  void operator=(const vtkTransmitImageDataPiece&); // Not implemented
};

#endif
