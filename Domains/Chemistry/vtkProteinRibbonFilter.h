/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProteinRibbonFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef _vtkProteinRibbonFilter_h
#define _vtkProteinRibbonFilter_h

// .NAME vtkProteinRibbonFilter - generates protein ribbons
// .SECTION Description
// vtkProteinRibbonFilter is an poly data algorithm which generates
// protein ribbons.

#include "vtkDomainsChemistryModule.h" // for export macro
#include "vtkPolyDataAlgorithm.h"

class VTKDOMAINSCHEMISTRY_EXPORT vtkProteinRibbonFilter
  : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkProteinRibbonFilter, vtkPolyDataAlgorithm)
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkProteinRibbonFilter* New();

protected:
  vtkProteinRibbonFilter();
  ~vtkProteinRibbonFilter();

  int FillInputPortInformation(int, vtkInformation*);

  int RequestData(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *);

private:
  vtkProteinRibbonFilter(const vtkProteinRibbonFilter&);  // Not implemented.
  void operator=(const vtkProteinRibbonFilter&);  // Not implemented.
};

#endif // _vtkProteinRibbonFilter_h
