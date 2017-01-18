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
/**
 * @class   vtkSimpleBondPerceiver
 * @brief   Create a simple guess of a molecule's
 * topology
 *
 *
 *
 * vtkSimpleBondPerceiver performs a simple check of all interatomic distances
 * and adds a single bond between atoms that are reasonably close. If the
 * interatomic distance is less than the sum of the two atom's covalent radii
 * plus a tolerance, a single bond is added.
 *
 *
 * @warning
 * This algorithm does not consider valences, hybridization, aromaticity, or
 * anything other than atomic separations. It will not produce anything other
 * than single bonds.
*/

#ifndef vtkSimpleBondPerceiver_h
#define vtkSimpleBondPerceiver_h

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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the tolerance used in the comparisons. (Default: 0.45)
   */
  vtkSetMacro(Tolerance, float);
  vtkGetMacro(Tolerance, float);
  //@}

protected:
  vtkSimpleBondPerceiver();
  ~vtkSimpleBondPerceiver() VTK_OVERRIDE;

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector) VTK_OVERRIDE;

  float Tolerance;

private:
  vtkSimpleBondPerceiver(const vtkSimpleBondPerceiver&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSimpleBondPerceiver&) VTK_DELETE_FUNCTION;
};

#endif
