// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridEvaluateCoarse
 * @brief   The value of the parent cell is determined from an operation
 * defined on the values of the child cells.
 *
 *
 * vtkHyperTreeGridEvaluateCoarse is a filter that takes as input an hypertree
 * grid.
 * The value of the parent cell (coarse) is determined from an operation
 * defined on the values of the child cells (refined cell).
 *
 * The predefined operators are :
 *  - OPERATOR_DON_T_CHANGE_FAST:
 *      Operator does not change coarse value (default), just shallow copy
 *  - OPERATOR_DON_T_CHANGE:
 *      Operator does not change coarse value but iterate over all cells,
 *      just shallow copy
 *  - OPERATOR_MIN:
 *      The littlest value of the unmasked child cells
 *  - OPERATOR_MAX:
 *      The biggest value of the unmasked child cells
 *  - OPERATOR_SUM:
 *      The sum of the values of the unmasked child cells
 *  - OPERATOR_AVERAGE:
 *      The average of the values of the child cells. If the cell is masked
 *      a value is put at "default value" (default value = 0 if not set
 *      by SetDefault).
 *  - OPERATOR_UNMASKED_AVERAGE:
 *      The average of the values of the unmasked child cells
 *  - OPERATOR_ELDER_CHILD:
 *      Operator puts the value of the first child (elder child)
 *  - OPERATOR_SPLATTING_AVERAGE:
 *      The splatting average of the values of the child cells. If the cell
 *      is masked a value is put at "default value" (default value = 0 if
 *      not set by SetDefault).
 *      The calculation of the average should normally be done by dividing by
 *      the number of children (GetNumberOfChildren) which is worth f^d where
 *      f, refinement factor and d, number of spatial dimension.
 *      In the calculation of the mean for splatting, the division
 *      involves f^(d-1).
 *      In 3D, if a mesh is refined into 8 child cells each having a value
 *      set to 0.5, all these children produce 4 splats of value 1. In fact,
 *      the value of the expected splat at the coarse cell (parent) is 1.
 *      But a standard average will give 0.5 (value / f^d). This is why
 *      the calculation of the average for splatting is different
 *      (value / f^(d-1)).
 *
 *
 * @sa
 * vtkHyperTreeGrid vtkHyperTreeGridAlgorithm
 *
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien, 2016-18
 * This class was modified to take in account the field values with components
 * different of one, by Florent Denef, 2019
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridEvaluateCoarse_h
#define vtkHyperTreeGridEvaluateCoarse_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

#include <vector> // For scratch storage.

VTK_ABI_NAMESPACE_BEGIN
class vtkBitArray;

class vtkHyperTreeGrid;

class vtkHyperTreeGridNonOrientedCursor;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridEvaluateCoarse : public vtkHyperTreeGridAlgorithm
{
public:
  enum
  {
    OPERATOR_DON_T_CHANGE_FAST = 0,
    OPERATOR_DON_T_CHANGE = 1,
    OPERATOR_MIN = 2,
    OPERATOR_MAX = 3,
    OPERATOR_SUM = 4,
    OPERATOR_AVERAGE = 5,
    OPERATOR_UNMASKED_AVERAGE = 6,
    OPERATOR_ELDER_CHILD = 7,
    OPERATOR_SPLATTING_AVERAGE = 8
  };

  static vtkHyperTreeGridEvaluateCoarse* New();
  vtkTypeMacro(vtkHyperTreeGridEvaluateCoarse, vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get reduction operator
   */
  vtkSetMacro(Operator, unsigned int);
  vtkGetMacro(Operator, unsigned int);
  ///@}

  /**
   * Set/Get default value, replacing missing values in average functions
   */
  vtkSetMacro(Default, double);

protected:
  vtkHyperTreeGridEvaluateCoarse();
  ~vtkHyperTreeGridEvaluateCoarse() override;

  /**
   * Main routine to extract hyper tree grid levels
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

  /**
   * Define default input and output port types
   */
  int FillOutputPortInformation(int, vtkInformation*) override;

  /**
   * Recursively descend into tree down to leaves
   */
  virtual void ProcessNode(vtkHyperTreeGridNonOrientedCursor*);

private:
  vtkHyperTreeGridEvaluateCoarse(const vtkHyperTreeGridEvaluateCoarse&) = delete;
  void operator=(const vtkHyperTreeGridEvaluateCoarse&) = delete;

  /**
   * Deep-copy node data
   */
  virtual void ProcessNodeNoChange(vtkHyperTreeGridNonOrientedCursor*);

  /**
   * Process recursively 'ichild' child and fill 'childrenValues' array.
   * The cell pointed by 'outCursor' needs to have at least 'ichild' children.
   */
  void ProcessChild(vtkHyperTreeGridNonOrientedCursor* outCursor, int ichild,
    std::vector<std::vector<double>>& childrenValues);

  /**
   * Dispatch list of children values to the right evaluation function,
   * as chosen with Operator.
   */
  virtual double EvalCoarse(const std::vector<double>&);

  ///@{
  /**
   * Functions evaluating the coarse cell value from the list of children's values
   */
  virtual double Min(const std::vector<double>&);
  virtual double Max(const std::vector<double>&);
  virtual double Sum(const std::vector<double>&);
  virtual double Average(const std::vector<double>&);
  virtual double UnmaskedAverage(const std::vector<double>&);
  virtual double ElderChild(const std::vector<double>&);
  virtual double SplattingAverage(const std::vector<double>&);
  ///@}

  unsigned int Operator = OPERATOR_DON_T_CHANGE;
  double Default = 0.0;
  unsigned int SplattingFactor = 1;
  unsigned int NumberOfChildren = 0;
  vtkBitArray* Mask = nullptr;
};

VTK_ABI_NAMESPACE_END
#endif // vtkHyperTreeGridEvaluateCoarse_h
