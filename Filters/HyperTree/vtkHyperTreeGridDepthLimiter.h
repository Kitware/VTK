/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridDepthLimiter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGridDepthLimiter
 * @brief   Hyper tree grid level extraction
 *
 *
 * Extract all levels down to a specified depth from a hyper tree grid.
 * If the required depth is greater or equal to the maximum level of the
 * input grid, then the output is identical.
 * Note that when a material mask is present, the geometry extent of the
 * output grid is guaranteed to contain that of the input tree, but the
 * former might be strictly larger than the latter. This is not a bug
 * but an expected behavior of which the user should be aware.
 *
 * @sa
 * vtkHyperTreeGrid vtkHyperTreeGridAlgorithm
 *
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien 2014
 * This class was modified by Philippe Pebay, 2016
 * This class was modified by Jacques-Bernard Lekien, 2018
 * This class was optimized by Jacques-Bernard Lekien, 2019,
 * by DepthLimiter directly manadged by HyperTreeGrid and (super)cursors.
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridDepthLimiter_h
#define vtkHyperTreeGridDepthLimiter_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

class vtkBitArray;
class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedCursor;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridDepthLimiter : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridDepthLimiter* New();
  vtkTypeMacro(vtkHyperTreeGridDepthLimiter, vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream&, vtkIndent) override;

  //@{
  /**
   * Set/Get True, create a new mask ; false, create a new HTG.
   */
  vtkSetMacro(JustCreateNewMask, bool);
  vtkGetMacro(JustCreateNewMask, bool);
  //@}

  //@{
  /**
   * Set/Get maximum depth to which output grid should be limited
   */
  vtkSetMacro(Depth, unsigned int);
  vtkGetMacro(Depth, unsigned int);
  //@}

protected:
  vtkHyperTreeGridDepthLimiter();
  ~vtkHyperTreeGridDepthLimiter() override;

  /**
   * For this algorithm the output is a vtkHyperTreeGrid instance
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

#endif // vtkHyperTreeGridDepthLimiter_h
