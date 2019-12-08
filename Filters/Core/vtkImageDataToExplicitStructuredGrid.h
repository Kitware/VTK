/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDataToExplicitStructuredGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageDataToExplicitStructuredGrid
 * @brief   Filter which converts a 3D image data into an explicit structured grid.
 */

#ifndef vtkImageDataToExplicitStructuredGrid_h
#define vtkImageDataToExplicitStructuredGrid_h

#include "vtkExplicitStructuredGridAlgorithm.h"
#include "vtkFiltersCoreModule.h" // For export macro

class VTKFILTERSCORE_EXPORT vtkImageDataToExplicitStructuredGrid
  : public vtkExplicitStructuredGridAlgorithm
{
public:
  static vtkImageDataToExplicitStructuredGrid* New();
  vtkTypeMacro(vtkImageDataToExplicitStructuredGrid, vtkExplicitStructuredGridAlgorithm);

protected:
  vtkImageDataToExplicitStructuredGrid() = default;
  ~vtkImageDataToExplicitStructuredGrid() override = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkImageDataToExplicitStructuredGrid(const vtkImageDataToExplicitStructuredGrid&) = delete;
  void operator=(const vtkImageDataToExplicitStructuredGrid&) = delete;
};

#endif
