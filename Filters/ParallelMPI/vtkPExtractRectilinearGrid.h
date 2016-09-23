/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPExtractRectilinearGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPExtractRectilinearGrid
 * @brief   Extract VOI and/or sub-sample a distributed
 *  rectilinear grid dataset.
 *
 *
 *  vtkPExtractRectilinearGrid inherits from vtkExtractVOI & provides additional
 *  functionality when dealing with a distributed dataset. Specifically, when
 *  sub-sampling a dataset, a gap may be introduced between partitions. This
 *  filter handles such cases correctly by growing the grid to the right to
 *  close the gap.
 *
 * @sa
 *  vtkExtractRectilinearGrid
*/

#ifndef vtkPExtractRectilinearGrid_h
#define vtkPExtractRectilinearGrid_h

#include "vtkFiltersParallelMPIModule.h" // For export macro
#include "vtkExtractRectilinearGrid.h"

// Forward Declarations
class vtkInformation;
class vtkInformationVector;
class vtkMPIController;

class VTKFILTERSPARALLELMPI_EXPORT vtkPExtractRectilinearGrid :
  public vtkExtractRectilinearGrid
{
public:
  static vtkPExtractRectilinearGrid* New();
  vtkTypeMacro(vtkPExtractRectilinearGrid,vtkExtractRectilinearGrid);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPExtractRectilinearGrid();
  virtual ~vtkPExtractRectilinearGrid();

  // Standard VTK Pipeline methods
  virtual int RequestData(
      vtkInformation*, vtkInformationVector**,vtkInformationVector*);
  virtual int RequestInformation(
      vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  virtual int RequestUpdateExtent(
      vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  vtkMPIController* Controller;

private:
  vtkPExtractRectilinearGrid(const vtkPExtractRectilinearGrid&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPExtractRectilinearGrid&) VTK_DELETE_FUNCTION;
};

#endif /* VTKPEXTRACTRECTILINEARGRID_H_ */
