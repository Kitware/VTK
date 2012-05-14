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
// .NAME vtkMoleculeToPolyDataFilter - abstract filter class
// .SECTION Description
// vtkMoleculeToPolyDataFilter is an abstract filter class whose
// subclasses take as input datasets of type vtkMolecule and
// generate polygonal data on output.

#ifndef __vtkMoleculeToPolyDataFilter_h
#define __vtkMoleculeToPolyDataFilter_h

#include "vtkDomainsChemistryModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkMolecule;

class VTKDOMAINSCHEMISTRY_EXPORT vtkMoleculeToPolyDataFilter
: public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkMoleculeToPolyDataFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkMolecule * GetInput();

protected:
  vtkMoleculeToPolyDataFilter();
  ~vtkMoleculeToPolyDataFilter();

  virtual int FillInputPortInformation(int, vtkInformation*);

private:
  vtkMoleculeToPolyDataFilter(const vtkMoleculeToPolyDataFilter&);  // Not implemented.
  void operator=(const vtkMoleculeToPolyDataFilter&);  // Not implemented.
};

#endif
