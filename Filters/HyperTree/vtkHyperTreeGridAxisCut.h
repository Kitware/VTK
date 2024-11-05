// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridAxisCut
 * @brief   Axis aligned hyper tree grid cut
 *
 * Cut an hyper tree grid along an axis aligned plane and output a hyper
 * tree grid lower dimensionality. Only works for 3D HTGs as input.
 *
 * @note This filter uses fuzzy comparison to test if a plane cuts the
 * HTG (used epsilon is DBL_EPSILON). It prevents having no cut generated
 * inside the HTG (when the is being coincident to cell faces) or bugs
 * related to floating point comparison.
 *
 * NB: This new (2014-16) version of the class is not to be confused with
 * earlier (2012-13) version that produced a vtkPolyData output composed of
 * disjoint (no point sharing) quadrilaterals, with possibly superimposed
 * faces when cut plane contained inter-cell boundaries.
 *
 * @sa
 * vtkHyperTreeGrid vtkHyperTreeGridAlgorithm
 *
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien 2014
 * This class was modified by Philippe Pebay, 2016
 * This class was modified by Jacques-Bernard Lekien, 2018
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridAxisCut_h
#define vtkHyperTreeGridAxisCut_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkBitArray;
class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedCursor;
class vtkHyperTreeGridNonOrientedGeometryCursor;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridAxisCut : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridAxisCut* New();
  vtkTypeMacro(vtkHyperTreeGridAxisCut, vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Normal axis: 0=X, 1=Y, 2=Z. Default is 0
   */
  vtkSetClampMacro(PlaneNormalAxis, int, 0, 2);
  vtkGetMacro(PlaneNormalAxis, int);
  ///@}

  ///@{
  /**
   * Position of plane: Axis constant. Default is 0.0
   */
  vtkSetMacro(PlanePosition, double);
  vtkGetMacro(PlanePosition, double);
  ///@}

protected:
  vtkHyperTreeGridAxisCut();
  ~vtkHyperTreeGridAxisCut() override;

  // For this algorithm the output is a vtkHyperTreeGrid instance
  int FillOutputPortInformation(int, vtkInformation*) override;

  /**
   * Main routine to generate hyper tree grid cut
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTree(vtkHyperTreeGridNonOrientedGeometryCursor* inCursor,
    vtkHyperTreeGridNonOrientedCursor* outCursor);

  /**
   * Direction of plane normal
   */
  int PlaneNormalAxis;

  /**
   * Intercept of plane along normal
   */
  double PlanePosition;
  double PlanePositionRealUse;

  /**
   * Output material mask constructed by this filter
   */
  vtkBitArray* InMask;
  vtkBitArray* OutMask;

  /**
   * Keep track of current index in output hyper tree grid
   */
  vtkIdType CurrentId;

private:
  vtkHyperTreeGridAxisCut(const vtkHyperTreeGridAxisCut&) = delete;
  void operator=(const vtkHyperTreeGridAxisCut&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkHyperTreeGridAxisCut_h
