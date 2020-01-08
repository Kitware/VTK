/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPExtractVOI.cxx

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
 *  vtkPExtractVOI inherits from vtkExtractVOI and provides additional
 *  functionality when dealing with a distributed dataset. Specifically, when
 *  sub-sampling a dataset, a gap may be introduced between partitions. This
 *  filter handles such cases correctly by growing the grid to the right to
 *  close the gap.
 *
 * @sa
 *  vtkExtractVOI
 */

#ifndef vtkPExtractVOI_h
#define vtkPExtractVOI_h

#include "vtkExtractVOI.h"
#include "vtkFiltersParallelMPIModule.h" // For export macro

// Forward Declarations
class vtkInformation;
class vtkInformationVector;
class vtkMPIController;

class VTKFILTERSPARALLELMPI_EXPORT vtkPExtractVOI : public vtkExtractVOI
{
public:
  static vtkPExtractVOI* New();
  vtkTypeMacro(vtkPExtractVOI, vtkExtractVOI);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPExtractVOI();
  ~vtkPExtractVOI() override;

  // Standard VTK Pipeline methods
  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  virtual int RequestInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  virtual int RequestUpdateExtent(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkMPIController* Controller;

private:
  vtkPExtractVOI(const vtkPExtractVOI&) = delete;
  void operator=(const vtkPExtractVOI&) = delete;
};

#endif
