/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedPolyDataIds.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExtractSelectedPolyDataIds
 * @brief   extract a list of cells from a polydata
 *
 * vtkExtractSelectedPolyDataIds extracts all cells in vtkSelection from a
 * vtkPolyData.
 * @sa
 * vtkSelection
*/

#ifndef vtkExtractSelectedPolyDataIds_h
#define vtkExtractSelectedPolyDataIds_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkSelection;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractSelectedPolyDataIds : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkExtractSelectedPolyDataIds,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkExtractSelectedPolyDataIds *New();

protected:
  vtkExtractSelectedPolyDataIds();
  ~vtkExtractSelectedPolyDataIds() VTK_OVERRIDE;

  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  // Usual data generation method
  int RequestData(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *) VTK_OVERRIDE;

private:
  vtkExtractSelectedPolyDataIds(const vtkExtractSelectedPolyDataIds&) VTK_DELETE_FUNCTION;
  void operator=(const vtkExtractSelectedPolyDataIds&) VTK_DELETE_FUNCTION;
};

#endif
