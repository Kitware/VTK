/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridScales.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGridScales
 * @brief   A specifalized type of vtkHyperTreeGrid for the case
 * when root cells have uniform sizes in each direction *
 *
 * @sa
 * vtkHyperTree vtkHyperTreeGrid vtkRectilinearGrid
 *
 * @par Thanks:
 * This class was written by Jacques-Bernard Lekien (CEA)
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridScales_h
#define vtkHyperTreeGridScales_h

#include <vector> // For std::vector

class vtkHyperTreeGridScales
{
public:
  /**
   * JB Construit cette classe a partir du scale de la maille
   * d'origine d'un HyperTree et du subdivision factor
   */
  vtkHyperTreeGridScales(double branchfactor, const double scale[3])
    : BranchFactor(branchfactor)
    , CurrentFailLevel(1)
    , CellScales(scale, scale + 3)
  {
  }

  ~vtkHyperTreeGridScales() = default;

  /**
   * JB Retourne le scale des mailles du niveau demande
   */
  double GetBranchFactor() const { return this->BranchFactor; }

  /**
   * JB Retourne le scale des mailles du niveau demande
   */
  double* GetScale(unsigned int level) const
  {
    this->Update(level);
    return this->CellScales.data() + 3 * level;
  }

  /**
   * JB
   */
  double GetScaleX(unsigned int level) const
  {
    this->Update(level);
    return this->CellScales[3 * level + 0];
  }

  /**
   * JB
   */
  double GetScaleY(unsigned int level) const
  {
    this->Update(level);
    return this->CellScales[3 * level + 1];
  }

  /**
   * JB
   */
  double GetScaleZ(unsigned int level) const
  {
    this->Update(level);
    return this->CellScales[3 * level + 2];
  }

  /**
   * JB Retourne le scale des mailles du niveau demande
   */
  void GetScale(unsigned int level, double scale[3]) const
  {
    this->Update(level);
    memcpy(scale, this->CellScales.data() + 3 * level, 3 * sizeof(double));
  }

  /**
   * JB
   */
  unsigned int GetCurrentFailLevel() const { return this->CurrentFailLevel; }

private:
  /**
   * JB Update the cell scales table afin de repondre que la
   * table puisse retourner la taille de la maille pour ce niveau
   * demande
   */
  void Update(unsigned int level) const
  {
    if (level < this->CurrentFailLevel)
    {
      return;
    }
    this->CurrentFailLevel = level + 1;
    this->CellScales.resize(3 * this->CurrentFailLevel);
    auto current = this->CellScales.begin() + 3 * (this->CurrentFailLevel - 1);
    auto previous = current - 3;
    auto end = this->CellScales.end();
    for (; current != end; ++current, ++previous)
    {
      *current = *previous / this->BranchFactor;
    }
  }

  /**
   * The subdivision factor in the grid refinement scheme
   */
  const double BranchFactor;

  /**
   * The cache cell scales table
   */
  mutable unsigned int CurrentFailLevel;
  mutable std::vector<double> CellScales;
};

#endif
// VTK-HeaderTest-Exclude: vtkHyperTreeGridScales.h
