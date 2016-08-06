/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinearToQuadraticCellsFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkLinearToQuadraticCellsFilter - degree elevate the cells of a linear unstructured grid.
//
// .SECTION Description
// vtkLinearToQuadraticCellsFilter takes an unstructured grid comprised of
// linear cells and degree elevates each of the cells to quadratic. Additional
// points are simply interpolated from the existing points (there is no snapping
// to an external model).

#ifndef vtkLinearToQuadraticCellsFilter_h
#define vtkLinearToQuadraticCellsFilter_h

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkIncrementalPointLocator;

class VTKFILTERSGEOMETRY_EXPORT vtkLinearToQuadraticCellsFilter :
  public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkLinearToQuadraticCellsFilter,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkLinearToQuadraticCellsFilter *New();

  // Description:
  // Specify a spatial locator for merging points. By default, an
  // instance of vtkMergePoints is used.
  void SetLocator(vtkIncrementalPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkIncrementalPointLocator);

  // Description:
  // Create default locator. Used to create one when none is specified. The
  // locator is used to merge coincident points.
  void CreateDefaultLocator();

  // Description:
  // Set/get the desired precision for the output types. See the documentation
  // for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
  // the available precision settings.
  // OutputPointsPrecision is DEFAULT_PRECISION by default.
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);

  // Description:
  // Return the mtime also considering the locator.
  vtkMTimeType GetMTime();

protected:
  vtkLinearToQuadraticCellsFilter();
  ~vtkLinearToQuadraticCellsFilter();

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *);

  vtkIncrementalPointLocator *Locator;
  int OutputPointsPrecision;

private:
  vtkLinearToQuadraticCellsFilter(const vtkLinearToQuadraticCellsFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkLinearToQuadraticCellsFilter&) VTK_DELETE_FUNCTION;

};

#endif
