/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMoleculeToBondStickFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMoleculeToBondStickFilter
 * @brief   Generate polydata with cylinders
 * representing bonds
*/

#ifndef vtkMoleculeToBondStickFilter_h
#define vtkMoleculeToBondStickFilter_h

#include "vtkDomainsChemistryModule.h" // For export macro
#include "vtkMoleculeToPolyDataFilter.h"

class vtkMolecule;

class VTKDOMAINSCHEMISTRY_EXPORT vtkMoleculeToBondStickFilter
: public vtkMoleculeToPolyDataFilter
{
 public:
  vtkTypeMacro(vtkMoleculeToBondStickFilter,vtkMoleculeToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  static vtkMoleculeToBondStickFilter *New();

protected:
  vtkMoleculeToBondStickFilter();
  ~vtkMoleculeToBondStickFilter() VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) VTK_OVERRIDE;

private:
  vtkMoleculeToBondStickFilter(const vtkMoleculeToBondStickFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMoleculeToBondStickFilter&) VTK_DELETE_FUNCTION;
};

#endif
