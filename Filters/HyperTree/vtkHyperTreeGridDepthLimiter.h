// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridDepthLimiter
 * @brief   Hyper tree grid level extraction
 *
 *
 * Extracts all levels down to a specified depth from a HyperTreeGrid
 * representation.
 * If the required depth is greater or equal to the maximum level of the
 * input grid, then the output is identical.
 *
 * @sa
 * vtkHyperTreeGrid vtkHyperTreeGridAlgorithm vtkUniformHyperTreeGrid
 *
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien 2014
 * This class was modified by Philippe Pebay, 2016
 * This class was modified, 2018, and optimized, 2019, by Jacques-Bernard Lekien,
 * by DepthLimiter directly managed by HyperTreeGrid and (super)cursors.
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridDepthLimiter_h
#define vtkHyperTreeGridDepthLimiter_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkBitArray;
class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedCursor;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridDepthLimiter : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridDepthLimiter* New();
  vtkTypeMacro(vtkHyperTreeGridDepthLimiter, vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get True, create a new mask ; False, create a new vtkHyperTreeGrid (HTG)
   * Actually, setting to true no longer creates a new mask
   * but sets an attribute of the HTG that is used in the HTG and sliders.
   * The name of this option is historical and being kept for retro-compatibility reasons.
   * Default is true.
   */
  vtkSetMacro(JustCreateNewMask, bool);
  vtkGetMacro(JustCreateNewMask, bool);
  ///@}

  ///@{
  /**
   * Set/Get maximum depth to which output grid should be limited
   * Default is 0.
   */
  vtkSetMacro(Depth, unsigned int);
  vtkGetMacro(Depth, unsigned int);
  ///@}

protected:
  vtkHyperTreeGridDepthLimiter();
  ~vtkHyperTreeGridDepthLimiter() override;

  /**
   * For this algorithm the output is a vtkHyperTreeGrid or
   * vtkUniformHyperTreeGrid instance
   */
  int FillOutputPortInformation(int, vtkInformation*) override;

  /**
   * Main routine to extract hyper tree grid levels
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTree(
    vtkHyperTreeGridNonOrientedCursor*, vtkHyperTreeGridNonOrientedCursor*);

  /**
   * Maximum depth of hyper tree grid to be extracted
   */
  unsigned int Depth;

  /**
   * Input mask
   */
  vtkBitArray* InMask;

  /**
   * Output mask constructed by this filter
   */
  vtkBitArray* OutMask;

  /**
   * Keep track of current index in output hyper tree grid
   */
  vtkIdType CurrentId;

  /**
   * With or without copy
   */
  bool JustCreateNewMask;

private:
  vtkHyperTreeGridDepthLimiter(const vtkHyperTreeGridDepthLimiter&) = delete;
  void operator=(const vtkHyperTreeGridDepthLimiter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkHyperTreeGridDepthLimiter_h
