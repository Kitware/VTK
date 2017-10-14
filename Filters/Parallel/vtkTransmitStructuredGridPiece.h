/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransmitStructuredGridPiece.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTransmitRectilinearGridPiece
 * @brief   Redistributes data produced
 * by serial readers
 *
 *
 * This filter can be used to redistribute data from producers that can't
 * produce data in parallel. All data is produced on first process and
 * the distributed to others using the multiprocess controller.
 *
 * Note that this class is legacy. The superclass does all the work and
 * can be used directly instead.
*/

#ifndef vtkTransmitStructuredGridPiece_h
#define vtkTransmitStructuredGridPiece_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkTransmitStructuredDataPiece.h"

class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkTransmitStructuredGridPiece : public vtkTransmitStructuredDataPiece
{
public:
  static vtkTransmitStructuredGridPiece *New();
  vtkTypeMacro(vtkTransmitStructuredGridPiece, vtkTransmitStructuredDataPiece);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkTransmitStructuredGridPiece();
  ~vtkTransmitStructuredGridPiece() override;

private:
  vtkTransmitStructuredGridPiece(const vtkTransmitStructuredGridPiece&) = delete;
  void operator=(const vtkTransmitStructuredGridPiece&) = delete;
};

#endif
