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
// .NAME vtkPExtractGrid - Extract VOI and/or sub-sample a distributed
//  structured dataset.
//
// .SECTION Description
//  vtkPExtractVOI inherits from vtkExtractVOI and provides additional
//  functionality when dealing with a distributed dataset. Specifically, when
//  sub-sampling a dataset, a gap may be introduced between partitions. This
//  filter handles such cases correctly by growing the grid to the right to
//  close the gap.
//
// .SECTION See Also
//  vtkExtractVOI

#ifndef vtkPExtractVOI_h
#define vtkPExtractVOI_h

#include "vtkFiltersParallelMPIModule.h" // For export macro
#include "vtkExtractVOI.h"

// Forward Declarations
class vtkInformation;
class vtkInformationVector;
class vtkMPIController;

class VTKFILTERSPARALLELMPI_EXPORT vtkPExtractVOI : public vtkExtractVOI
{
public:
  static vtkPExtractVOI* New();
  vtkTypeMacro(vtkPExtractVOI,vtkExtractVOI);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPExtractVOI();
  virtual ~vtkPExtractVOI();

  // Standard VTK Pipeline methods
  virtual int RequestData(
      vtkInformation*, vtkInformationVector**,vtkInformationVector*);
  virtual int RequestInformation(
      vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  virtual int RequestUpdateExtent(
      vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  vtkMPIController* Controller;

private:
  vtkPExtractVOI(const vtkPExtractVOI&); // Not implemented.
  void operator=(const vtkPExtractVOI&); // Not implemented.
};

#endif
