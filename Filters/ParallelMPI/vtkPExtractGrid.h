/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPExtractGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPExtractGrid
 * @brief   Extract VOI and/or sub-sample a distributed
 *  structured dataset.
 *
 *
 *  vtkPExtractGrid inherits from vtkExtractGrid and provides additional
 *  functionality when dealing with a distributed dataset. Specifically, when
 *  sub-sampling a dataset, a gap may be introduced between partitions. This
 *  filter handles such cases correctly by growing the grid to the right to
 *  close the gap.
 *
 * @sa
 *  vtkExtractGrid
*/

#ifndef vtkPExtractGrid_h
#define vtkPExtractGrid_h

#include "vtkFiltersParallelMPIModule.h" // For export macro
#include "vtkExtractGrid.h"

// Forward declarations
class vtkMPIController;

class VTKFILTERSPARALLELMPI_EXPORT vtkPExtractGrid: public vtkExtractGrid
{
public:
    static vtkPExtractGrid* New();
    vtkTypeMacro(vtkPExtractGrid,vtkExtractGrid);
    void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
    vtkPExtractGrid();
    virtual ~vtkPExtractGrid();

    // Standard VTK Pipeline methods
    virtual int RequestData(
        vtkInformation*, vtkInformationVector**,vtkInformationVector*) VTK_OVERRIDE;
    virtual int RequestInformation(
        vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;
    virtual int RequestUpdateExtent(
        vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

    vtkMPIController* Controller;

private:
    vtkPExtractGrid(const vtkPExtractGrid&) VTK_DELETE_FUNCTION;
    void operator=(const vtkPExtractGrid&) VTK_DELETE_FUNCTION;
};

#endif
