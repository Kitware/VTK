/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMoleculeToPolyDataFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMoleculeToPolyDataFilter
 * @brief   abstract filter class
 *
 * vtkMoleculeToPolyDataFilter is an abstract filter class whose
 * subclasses take as input datasets of type vtkMolecule and
 * generate polygonal data on output.
*/

#ifndef vtkMoleculeToPolyDataFilter_h
#define vtkMoleculeToPolyDataFilter_h

#include "vtkDomainsChemistryModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkMolecule;

class VTKDOMAINSCHEMISTRY_EXPORT vtkMoleculeToPolyDataFilter
: public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkMoleculeToPolyDataFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkMolecule * GetInput();

protected:
  vtkMoleculeToPolyDataFilter();
  ~vtkMoleculeToPolyDataFilter() override;

  int FillInputPortInformation(int, vtkInformation*) override;

private:
  vtkMoleculeToPolyDataFilter(const vtkMoleculeToPolyDataFilter&) = delete;
  void operator=(const vtkMoleculeToPolyDataFilter&) = delete;
};

#endif
