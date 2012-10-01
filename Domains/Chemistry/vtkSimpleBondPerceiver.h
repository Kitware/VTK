/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimpleBondPerceiver.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSimpleBondPerceiver - Create a simple guess of a molecule's
// topology

// .SECTION Description

// vtkSimpleBondPerceiver performs a simple check of all interatomic distances
// and adds a single bond between atoms that are reasonably close. If the
// interatomic distance is less than the sum of the two atom's covalent radii
// plus a tolerance, a single bond is added.

// .SECTION Caveats

// This algorithm does not consider valences, hybridization, aromaticity, or
// anything other than atomic separations. It will not produce anything other
// than single bonds.

#ifndef __vtkSimpleBondPerceiver_h
#define __vtkSimpleBondPerceiver_h

#include "vtkDomainsChemistryModule.h" // For export macro
#include "vtkMoleculeAlgorithm.h"

class vtkDataSet;
class vtkMolecule;

class VTKDOMAINSCHEMISTRY_EXPORT vtkSimpleBondPerceiver :
    public vtkMoleculeAlgorithm
{
public:
  static vtkSimpleBondPerceiver *New();
  vtkTypeMacro(vtkSimpleBondPerceiver,vtkMoleculeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the tolerance used in the comparisons. (Default: 0.45)
  vtkSetMacro(Tolerance, float);
  vtkGetMacro(Tolerance, float);

protected:
  vtkSimpleBondPerceiver();
  ~vtkSimpleBondPerceiver();

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  float Tolerance;

private:
  vtkSimpleBondPerceiver(const vtkSimpleBondPerceiver&);  // Not implemented.
  void operator=(const vtkSimpleBondPerceiver&);  // Not implemented.
};

#endif
