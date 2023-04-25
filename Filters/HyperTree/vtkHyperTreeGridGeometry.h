// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridGeometry
 * @brief   Hyper tree grid outer surface
 *
 * The value of the Orientation property associated with the HTG takes on the following meanings:
 * - in 1D:
 *   This value indicates the axis on which the HTG 1D is oriented.
 * - in 2D:
 *   This value indicates the plane on which the HTG 1D is oriented as:
 *   - value 0 describe a plane YZ (axis1=1 as Y, axis2=2 as Z);
 *   - value 1 describe a plane XZ (axis1=0 as X, axis2=2 as Z);
 *   - value 2 describe a plane XY (axis1=0 as X, axis2=1 as Y).
 * - in 3D: does not have to be, always XYZ
 *
 * The values of the coarse cell are taken into account in order to prune
 * when this coarse is masked or ghosted, this means that we ignore all
 * the child cells (coarse or leaves).
 * This is done even if one of his daughters would not have been explicitly
 * masked or ghosted.
 * Hence the importance of being consistent in terms of the value associated
 * with a coarse cell.
 *
 * COMMENT EST CONSTRUIT LE PURE MASK ET QUEL EST SON APPORT ET UTILISATION/EXPLOITATION.
 *
 * LA NOTION D'INTERFACE PLAN PARALLELE ETC
 * mixed cell ...
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
 * the subcell notion of onion skin interface (mixted cell), 2023
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
   * Turn on/off merging of coincident points. Note that is merging is
   * on, points with different point attributes (e.g., normals) are merged,
   * which may cause rendering artifacts.
   */
  vtkSetMacro(Merging, bool);
  vtkGetMacro(Merging, bool);
  ///@}

  //@{
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
  //@}

protected:
  vtkHyperTreeGridGeometry();
  ~vtkHyperTreeGridGeometry() override;

  /**
   * For this algorithm the output is a vtkPolyData instance
   */
  int FillOutputPortInformation(int, vtkInformation*) override;

  /**
   * Main routine to generate external boundary
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

  /**
   * Storage for points of output unstructured mesh
   */
  vtkPoints* Points;

  /**
   * Storage for cells of output unstructured mesh
   */
  vtkCellArray* Cells;

  /**
   * Boolean for passing cell ids to poly data
   *
   * default is false
   */
  bool PassThroughCellIds = false;

  /**
   * Name of the array holding original cell ids in output if PassThroughCellIds is true
   */
  std::string OriginalCellIdArrayName = "vtkOriginalCellIds";

  /**
   *JB Un locator est utilise afin de produire un maillage avec moins
   *JB de points. Le gain en 3D est de l'ordre d'un facteur 4 !
   */
  bool Merging;

private:
  vtkHyperTreeGridGeometry(const vtkHyperTreeGridGeometry&) = delete;
  void operator=(const vtkHyperTreeGridGeometry&) = delete;

public: // pas pour une bonne raison
  class vtkInternal;
  class vtkInternal1D;
  class vtkInternal2D;
  class vtkInternal3D;
  vtkInternal* Internal = nullptr;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkHyperTreeGridGeometry_h */
