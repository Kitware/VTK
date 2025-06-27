// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridScales
 * @brief   Dynamic generation of scales for vtkHyperTree
 *
 * Given a level 0 scame, compute & cache cell scales for lower levels.
 *
 * @sa
 * vtkHyperTree vtkHyperTreeGrid
 *
 * @par Thanks:
 * This class was written by Jacques-Bernard Lekien (CEA)
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridScales_h
#define vtkHyperTreeGridScales_h

#include "vtkABINamespace.h"
#include "vtkDeprecation.h" // VTK_DEPRECATED_IN_9_6_0

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

  VTK_DEPRECATED_IN_9_6_0("This function is deprecated, Use ComputeScale instead")
  double* GetScale(unsigned int level) { return ComputeScale(level); };

  double* ComputeScale(unsigned int level)
  {
    this->Update(level);
    return this->CellScales.data() + 3 * level;
  }

  VTK_DEPRECATED_IN_9_6_0("This function is deprecated, Use ComputeScaleX instead")
  double GetScaleX(unsigned int level) { return ComputeScaleX(level); };

  double ComputeScaleX(unsigned int level)
  {
    this->Update(level);
    return this->CellScales[3 * level + 0];
  }

  VTK_DEPRECATED_IN_9_6_0("This function is deprecated, Use ComputeScaleY instead")
  double GetScaleY(unsigned int level) { return ComputeScaleY(level); };

  double ComputeScaleY(unsigned int level)
  {
    this->Update(level);
    return this->CellScales[3 * level + 1];
  }

  VTK_DEPRECATED_IN_9_6_0("This function is deprecated, Use ComputeScaleZ instead")
  double GetScaleZ(unsigned int level) { return ComputeScaleZ(level); };

  double ComputeScaleZ(unsigned int level)
  {
    this->Update(level);
    return this->CellScales[3 * level + 2];
  }

  VTK_DEPRECATED_IN_9_6_0("This function is deprecated, Use ComputeScale instead")
  void GetScale(unsigned int level, double scale[3]) { ComputeScale(level, scale); };

  /**
   * Return the mesh scale at the given level
   */
  void ComputeScale(unsigned int level, double scale[3])
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
  void Update(unsigned int level)
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
  unsigned int CurrentFailLevel;
  std::vector<double> CellScales;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkHyperTreeGridScales.h
