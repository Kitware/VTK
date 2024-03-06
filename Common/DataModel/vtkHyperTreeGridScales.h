// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkABINamespace.h"

#include <cstring> // For memcpy
#include <vector>  // For std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkHyperTreeGridScales
{
public:
  /**
   * Build this class from the original scale mesh and subdivision factor
   */
  vtkHyperTreeGridScales(double branchfactor, const double scale[3])
    : BranchFactor(branchfactor)
    , CurrentFailLevel(1)
    , CellScales(scale, scale + 3)
  {
  }

  ~vtkHyperTreeGridScales() = default;

  double GetBranchFactor() const { return this->BranchFactor; }

  double* GetScale(unsigned int level) const
  {
    this->Update(level);
    return this->CellScales.data() + 3 * level;
  }

  double GetScaleX(unsigned int level) const
  {
    this->Update(level);
    return this->CellScales[3 * level + 0];
  }

  double GetScaleY(unsigned int level) const
  {
    this->Update(level);
    return this->CellScales[3 * level + 1];
  }

  double GetScaleZ(unsigned int level) const
  {
    this->Update(level);
    return this->CellScales[3 * level + 2];
  }

  /**
   * Return the mesh scale at the given level
   */
  void GetScale(unsigned int level, double scale[3]) const
  {
    this->Update(level);
    memcpy(scale, this->CellScales.data() + 3 * level, 3 * sizeof(double));
  }

  unsigned int GetCurrentFailLevel() const { return this->CurrentFailLevel; }

private:
  vtkHyperTreeGridScales(const vtkHyperTreeGridScales&) = delete;
  vtkHyperTreeGridScales& operator=(const vtkHyperTreeGridScales&) = delete;

  /**
   * Update the cell scale table in order for the table to return the mesh at the given level.
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

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkHyperTreeGridScales.h
