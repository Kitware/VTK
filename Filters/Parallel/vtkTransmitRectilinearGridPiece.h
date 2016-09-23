/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransmitRectilinearGridPiece.h

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

#ifndef vtkTransmitRectilinearGridPiece_h
#define vtkTransmitRectilinearGridPiece_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkTransmitStructuredDataPiece.h"

class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkTransmitRectilinearGridPiece : public vtkTransmitStructuredDataPiece
{
public:
  static vtkTransmitRectilinearGridPiece *New();
  vtkTypeMacro(vtkTransmitRectilinearGridPiece, vtkTransmitStructuredDataPiece);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkTransmitRectilinearGridPiece();
  ~vtkTransmitRectilinearGridPiece();

private:
  vtkTransmitRectilinearGridPiece(const vtkTransmitRectilinearGridPiece&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTransmitRectilinearGridPiece&) VTK_DELETE_FUNCTION;
};

#endif
