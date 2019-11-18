/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridThreshold.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGridThreshold
 * @brief   Extract cells from a hyper tree grid
 * where selected scalar value is within given range.
 *
 *
 * This filter extracts cells from a hyper tree grid that satisfy the
 * following threshold: a cell is considered to be within range if its
 * value for the active scalar is within a specified range (inclusive).
 * The output remains a hyper tree grid.
 * JB Un parametre (JustCreateNewMask=true) permet de ne pas faire
 * le choix de la creation d'un nouveau HTG mais
 * de redefinir juste le masque.
 *
 * @sa
 * vtkHyperTreeGrid vtkHyperTreeGridAlgorithm vtkThreshold
 *
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien 2014
 * This class was revised by Philippe Pebay, 2016
 * This class was optimized by Jacques-Bernard Lekien, 2018.
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridThreshold_h
#define vtkHyperTreeGridThreshold_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

class vtkBitArray;
class vtkHyperTreeGrid;

class vtkHyperTreeGridNonOrientedCursor;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridThreshold : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridThreshold* New();
  vtkTypeMacro(vtkHyperTreeGridThreshold, vtkHyperTreeGridAlgorithm);
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
   * Set/Get minimum scalar value of threshold
   */
  vtkSetMacro(LowerThreshold, double);
  vtkGetMacro(LowerThreshold, double);
  //@}

  //@{
  /**
   * Set/Get maximum scalar value of threshold
   */
  vtkSetMacro(UpperThreshold, double);
  vtkGetMacro(UpperThreshold, double);
  //@}

  /**
   * Convenience method to set both threshold values at once
   */
  void ThresholdBetween(double, double);

protected:
  vtkHyperTreeGridThreshold();
  ~vtkHyperTreeGridThreshold() override;

  /**
   * For this algorithm the output is a vtkHyperTreeGrid instance
   */
  int FillOutputPortInformation(int, vtkInformation*) override;

  /**
   * Main routine to extract cells based on thresholded value
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

  /**
   * Recursively descend into tree down to leaves
   */
  bool RecursivelyProcessTree(
    vtkHyperTreeGridNonOrientedCursor*, vtkHyperTreeGridNonOrientedCursor*);
  bool RecursivelyProcessTreeWithCreateNewMask(vtkHyperTreeGridNonOrientedCursor*);

  /**
   * LowerThreshold scalar value to be accepted
   */
  double LowerThreshold;

  /**
   * UpperThreshold scalar value to be accepted
   */
  double UpperThreshold;

  /**
   * Input material mask
   */
  vtkBitArray* InMask;

  /**
   * Output material mask constructed by this filter
   */
  vtkBitArray* OutMask;

  /**
   * Keep track of current index in output hyper tree grid
   */
  vtkIdType CurrentId;

  /**
   * Keep track of selected input scalars
   */
  vtkDataArray* InScalars;

  /**
   * With or without copy
   */
  bool JustCreateNewMask;

private:
  vtkHyperTreeGridThreshold(const vtkHyperTreeGridThreshold&) = delete;
  void operator=(const vtkHyperTreeGridThreshold&) = delete;
};

#endif /* vtkHyperTreeGridThreshold */
