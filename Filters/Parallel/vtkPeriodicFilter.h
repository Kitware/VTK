/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPeriodicFiler.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkPeriodicFilter
 * @brief   A filter to produce mapped  periodic
 * multiblock dataset from a single block
 *
 *
 * Generate periodic dataset by transforming points, vectors, tensors
 * data arrays from an original data array.
 * The generated dataset is of the same type than the input (float or double).
 * This is an abstract class which do not implement the actual transformation.
 * Point coordinates are transformed, as well as all vectors (3-components) and
 * tensors (9 components) in points and cell data arrays.
 * The generated multiblock will have the same tree architecture than the input,
 * except transformed leaves are replaced by a vtkMultipieceDataSet.
 * Supported input leaf dataset type are: vtkPolyData, vtkStructuredGrid
 * and vtkUnstructuredGrid. Other data objects are transformed using the
 * transform filter (at a high cost!).
*/

#ifndef vtkPeriodicFilter_h
#define vtkPeriodicFilter_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

#include <set> // For block selection
#include <vector> // For pieces number

class vtkCompositeDataIterator;
class vtkCompositeDataSet;
class vtkDataObjectTreeIterator;
class vtkMultiPieceDataSet;

#define VTK_ITERATION_MODE_DIRECT_NB 0    // Generate a user-provided number of periods
#define VTK_ITERATION_MODE_MAX       1    // Generate a maximum of periods, i.e. a full period.

class VTKFILTERSPARALLEL_EXPORT vtkPeriodicFilter : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkPeriodicFilter, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set/Get Iteration mode.
   * VTK_ITERATION_MODE_DIRECT_NB to specify the number of periods,
   * VTK_ITERATION_MODE_MAX to generate a full period (default).
   */
  vtkSetClampMacro(IterationMode, int,
                   VTK_ITERATION_MODE_DIRECT_NB,
                   VTK_ITERATION_MODE_MAX);
  vtkGetMacro(IterationMode, int);
  void SetIterationModeToDirectNb()
    { this->SetIterationMode(VTK_ITERATION_MODE_DIRECT_NB); }
  void SetIterationModeToMax()
    { this->SetIterationMode(VTK_ITERATION_MODE_MAX); }
  //@}

  //@{
  /**
   * Set/Get Number of periods.
   * Used only with ITERATION_MODE_DIRECT_NB.
   */
  vtkSetMacro(NumberOfPeriods, int);
  vtkGetMacro(NumberOfPeriods, int);
  //@}

  /**
   * Select the periodic pieces indices.
   * Each node in the multi - block tree is identified by an \c index. The index can
   * be obtained by performing a preorder traversal of the tree (including empty
   * nodes). eg. A(B (D, E), C(F, G)).
   * Inorder traversal yields: A, B, D, E, C, F, G
   * Index of A is 0, while index of C is 4.
   */
  virtual void AddIndex(unsigned int index);

  /**
   * Remove an index from selected indices tress
   */
  virtual void RemoveIndex(unsigned int index);

  /**
   * Clear selected indices tree
   */
  virtual void RemoveAllIndices();

protected:
  vtkPeriodicFilter();
  ~vtkPeriodicFilter() VTK_OVERRIDE;

  // see algorithm for more info
  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;

  /**
   * Create a periodic data, leaf of the tree
   */
  virtual void CreatePeriodicDataSet(vtkCompositeDataIterator* loc,
                                     vtkCompositeDataSet* output,
                                     vtkCompositeDataSet* input) = 0;

  /**
   * Manually set the number of period on a specific leaf
   */
  virtual void SetPeriodNumber(vtkCompositeDataIterator* loc,
                               vtkCompositeDataSet* output,
                               int nbPeriod) = 0;

  std::vector<int> PeriodNumbers;     // Periods numbers by leaf
  bool ReducePeriodNumbers;

private:
  vtkPeriodicFilter(const vtkPeriodicFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPeriodicFilter&) VTK_DELETE_FUNCTION;

  int IterationMode;
  int NumberOfPeriods;      // User provided number of periods

  std::set<vtkIdType> Indices;          // Selected indices
};

#endif
