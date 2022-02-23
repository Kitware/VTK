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
 *
 * @sa
 * vtkSelection
 *
 * @deprecated vtkExtractSelectedPolyDataIds is deprecated in VTK 9.2 and will be removed.
 * Use `vtkExtractSelection` instead of `vtkExtractSelectedPolyDataIds`.
 *
 * Example using vtkExtractSelectedPolyDataIds:
 *
 * vtkNew<vtkExtractSelectedPolyDataIds> selFilter;
 * selFilter->SetInputConnection(0, sphereSource->GetOutputPort());
 * selFilter->SetInputConnection(1, selectionSource->GetOutputPort());
 *
 * Example using vtkExtractSelection:
 *
 * vtkNew<vtkExtractSelection> selFilter;
 * selFilter->SetInputConnection(0, sphereSource->GetOutputPort());
 * selFilter->SetInputConnection(1, selectionSource->GetOutputPort());
 *
 * convert selFilter's output from vtkUnstructuredGrid to vtkPolydata
 */

#ifndef vtkExtractSelectedPolyDataIds_h
#define vtkExtractSelectedPolyDataIds_h

#include "vtkDeprecation.h"             // For VTK_DEPRECATED_IN_9_2_0
#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkSelection;

class VTK_DEPRECATED_IN_9_2_0("Use vtkExtractSelection instead of vtkExtractSelectedPolyDataIds.")
  VTKFILTERSEXTRACTION_EXPORT vtkExtractSelectedPolyDataIds : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkExtractSelectedPolyDataIds, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkExtractSelectedPolyDataIds* New();

protected:
  vtkExtractSelectedPolyDataIds();
  ~vtkExtractSelectedPolyDataIds() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkExtractSelectedPolyDataIds(const vtkExtractSelectedPolyDataIds&) = delete;
  void operator=(const vtkExtractSelectedPolyDataIds&) = delete;
};

#endif

// VTK-HeaderTest-Exclude: vtkExtractSelectedPolyDataIds.h
