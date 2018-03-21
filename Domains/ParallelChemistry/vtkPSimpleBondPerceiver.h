/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPSimpleBondPerceiver.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Extension to the Paraview's chemistry features developped by Aymeric Pellé from Université de
// Technologie de Compiègne, France. <br>
// Contributed by Laurent Colombet (laurent.colombet@cea.fr) and Thierry Carrard
// (thierry.carrard@cea.fr) from
// CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France, BP12, F-91297 Arpajon,
// France. <br>

/**
 * @class   vtkPSimpleBondPerceiver
 * @brief   Create a simple guess of a molecule's topology
 *
 *
 * vtkPSimpleBondPerceiver is the parallel version of vtkSimpleBondPerceiver.
 * It computes ghost atoms, ghost bonds and then it calls algorithm from the serial version.
 *
 */

#ifndef vtkPSimpleBondPerceiver_h
#define vtkPSimpleBondPerceiver_h

#include "vtkDomainsParallelChemistryModule.h" // For export macro
#include "vtkSimpleBondPerceiver.h"

class VTKDOMAINSPARALLELCHEMISTRY_EXPORT vtkPSimpleBondPerceiver : public vtkSimpleBondPerceiver
{
public:
  static vtkPSimpleBondPerceiver* New();
  vtkTypeMacro(vtkPSimpleBondPerceiver, vtkSimpleBondPerceiver);

protected:
  vtkPSimpleBondPerceiver() = default;
  ~vtkPSimpleBondPerceiver() = default;

  /**
   * Create ghosts level in molecule.
   * Return true if ghosts are correctly initialized.
   */
  bool CreateGhosts(vtkMolecule* molecule);

  /**
   * Compute the bonds. Reimplements Superclass to create ghost before.
   */
  void ComputeBonds(vtkMolecule* molecule) override;

private:
  vtkPSimpleBondPerceiver(const vtkPSimpleBondPerceiver&) = delete; // Not implemented.
  void operator=(const vtkPSimpleBondPerceiver&) = delete;          // Not implemented.
};
#endif
