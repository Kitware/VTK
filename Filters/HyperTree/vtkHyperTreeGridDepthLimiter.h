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
 * This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)
*/

#ifndef vtkHyperTreeGridDepthLimiter_h
#define vtkHyperTreeGridDepthLimiter_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

class vtkBitArray;
class vtkHyperTreeCursor;
class vtkHyperTreeGrid;
class vtkHyperTreeGridCursor;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridDepthLimiter : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridDepthLimiter* New();
  vtkTypeMacro( vtkHyperTreeGridDepthLimiter, vtkHyperTreeGridAlgorithm );
  void PrintSelf( ostream&, vtkIndent ) override;

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
  int FillOutputPortInformation( int, vtkInformation* ) override;

  /**
   * Main routine to extract hyper tree grid levels
   */
  int ProcessTrees( vtkHyperTreeGrid*, vtkDataObject* ) override;

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTree( vtkHyperTreeGridCursor*,
                               vtkHyperTreeCursor*,
                               vtkBitArray* );

  /**
   * Maximum depth of hyper tree grid to be extracted
   */
  unsigned int Depth;

  /**
   * Output material mask constructed by this filter
   */
  vtkBitArray* MaterialMask;

  /**
   * Keep track of current index in output hyper tree grid
   */
  vtkIdType CurrentId;

private:
  vtkHyperTreeGridDepthLimiter(const vtkHyperTreeGridDepthLimiter&) = delete;
  void operator=(const vtkHyperTreeGridDepthLimiter&) = delete;
};

#endif /* vtkHyperTreeGridDepthLimiter_h */
