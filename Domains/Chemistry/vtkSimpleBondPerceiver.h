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
class vtkPeriodicTable;

class VTKDOMAINSCHEMISTRY_EXPORT vtkSimpleBondPerceiver :
    public vtkMoleculeAlgorithm
{
public:
  static vtkSimpleBondPerceiver *New();
  vtkTypeMacro(vtkSimpleBondPerceiver,vtkMoleculeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/Get the tolerance used in the comparisons. (Default: 0.45)
   */
  vtkSetMacro(Tolerance, float);
  vtkGetMacro(Tolerance, float);
  //@}

  //@{
  /**
   * Set/Get if the tolerance is absolute (i.e. added to radius)
   * or not (i.e. multiplied with radius). Default is true.
   */
  vtkGetMacro(IsToleranceAbsolute, bool);
  vtkSetMacro(IsToleranceAbsolute, bool);
  //@}

protected:
  vtkSimpleBondPerceiver();
  ~vtkSimpleBondPerceiver() override;

  int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector) override;

  /**
   * Compute the bonds of input molecule.
   */
  virtual void ComputeBonds(vtkMolecule* molecule);

  /**
   * Get the covalent radius corresponding to atomic number, modulated by Tolerance.
   * Tolerance is multiplied if IsToleranceAbsolute is false.
   * Half Tolerance is added if IsToleranceAbsolute is true (for backward compatibility)
   */
  double GetCovalentRadiusWithTolerance(vtkPeriodicTable* table, vtkIdType atomicNumber);

  float Tolerance;
  bool IsToleranceAbsolute;

private:
  vtkSimpleBondPerceiver(const vtkSimpleBondPerceiver&) = delete;
  void operator=(const vtkSimpleBondPerceiver&) = delete;
};

#endif
