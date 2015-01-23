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
// .NAME vtkExtractSelectedPolyDataIds - extract a list of cells from a polydata
// .SECTION Description
// vtkExtractSelectedPolyDataIds extracts all cells in vtkSelection from a
// vtkPolyData.
// .SECTION See Also
// vtkSelection

#ifndef vtkExtractSelectedPolyDataIds_h
#define vtkExtractSelectedPolyDataIds_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkSelection;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractSelectedPolyDataIds : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkExtractSelectedPolyDataIds,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkExtractSelectedPolyDataIds *New();

protected:
  vtkExtractSelectedPolyDataIds();
  ~vtkExtractSelectedPolyDataIds();

  int FillInputPortInformation(int port, vtkInformation* info);

  // Usual data generation method
  int RequestData(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *);

private:
  vtkExtractSelectedPolyDataIds(const vtkExtractSelectedPolyDataIds&);  // Not implemented.
  void operator=(const vtkExtractSelectedPolyDataIds&);  // Not implemented.
};

#endif
