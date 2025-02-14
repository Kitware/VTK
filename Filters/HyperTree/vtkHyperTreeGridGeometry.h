// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridGeometry
 * @brief   Generate vtkHyperTreeGrid external surface
 *
 * Generate the external surface (vtkPolyData) of the input 1D/2D/3D vtkHyperTreeGrid.
 * Delegate the work internally to different implementation classes depending on the
 * dimension of the input HyperTreeGrid.
 *
 * In the HTG case the surface generated is:
 * - 1D from a 1D HTG (segments)
 * - 2D from a 2D and 3D HTG (faces)
 *
 * This filters also take account of interfaces, that will generate "cuts"
 * over the generated segments/surfaces.
 *
 * @sa
 * vtkHyperTreeGrid vtkHyperTreeGridAlgorithm
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, Joachim Pouderoux, and Charles Law, Kitware 2013
 * This class was modified by Guenole Harel and Jacques-Bernard Lekien, 2014
 * This class was rewritten by Philippe Pebay, 2016
 * This class was modified by Jacques-Bernard Lekien and Guenole Harel, 2018
 * This class has been corrected and extended by Jacques-Bernard Lekien to fully support
 * the subcell notion of onion skin interface (mixed cell), 2023
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridGeometry_h
#define vtkHyperTreeGridGeometry_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkCellArray;
class vtkDataObject;
class vtkHyperTreeGrid;
class vtkInformation;
class vtkPoints;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridGeometry : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridGeometry* New();
  vtkTypeMacro(vtkHyperTreeGridGeometry, vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Turn on/off merging of coincident points using a locator.
   * Note that when merging is on, points with different point attributes
   * (e.g., normals) are merged, which may cause rendering artifacts.
   */
  vtkSetMacro(Merging, bool);
  vtkGetMacro(Merging, bool);
  ///@}

  ///@{
  /**
   * Set/Get for the PassThroughCellIds boolean.
   *
   * When set to true this boolean ensures an array named with whatever is
   * in `OriginalCellIdArrayName` gets created in the output holding the
   * original cell ids
   *
   * default is false
   */
  vtkSetMacro(PassThroughCellIds, bool);
  vtkGetMacro(PassThroughCellIds, bool);
  vtkBooleanMacro(PassThroughCellIds, bool);
  ///@}

  ///@{
  /**
   * Set/Get the OriginalCellIdArrayName string.
   *
   * When PassThroughCellIds is set to true, the name of the generated
   * array is whatever is set in this variable.
   *
   * default to vtkOriginalCellIds
   */
  vtkSetMacro(OriginalCellIdArrayName, std::string);
  vtkGetMacro(OriginalCellIdArrayName, std::string);
  ///@}

  ///@{
  /**
   * If false, only draw the interface (lines).
   * Otherwise, draw the full cell with interface (poly).
   *
   * default is true
   */
  vtkSetMacro(FillMaterial, bool);
  vtkGetMacro(FillMaterial, bool);
  vtkBooleanMacro(FillMaterial, bool);
  ///@}

protected:
  vtkHyperTreeGridGeometry() = default;
  ~vtkHyperTreeGridGeometry() override = default;

  /**
   * For this algorithm the output is a vtkPolyData instance
   */
  int FillOutputPortInformation(int, vtkInformation*) override;

  /**
   * Main routine to generate external boundary
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

  /**
   * Boolean for passing input original cell ids to the output poly data
   *
   * default is false
   */
  bool PassThroughCellIds = false;

  /**
   * Name of the array holding original cell ids in output if PassThroughCellIds is true
   */
  std::string OriginalCellIdArrayName = "vtkOriginalCellIds";

  /**
   * If true, a vtkIncrementalPointLocator is used when inserting
   * new points to the output.
   *
   * XXX: Only used in the 3D case.
   */
  bool Merging = false;

private:
  vtkHyperTreeGridGeometry(const vtkHyperTreeGridGeometry&) = delete;
  void operator=(const vtkHyperTreeGridGeometry&) = delete;

  bool FillMaterial = true;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkHyperTreeGridGeometry_h */
