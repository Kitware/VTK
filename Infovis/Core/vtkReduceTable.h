/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReduceTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkReduceTable - combine some of the rows of a table
//
// .SECTION Description
// Collapses the rows of the input table so that one particular
// column (the IndexColumn) does not contain any duplicate values.
// Thus the output table will have the same columns as the input
// table, but potentially fewer rows.  One example use of this
// class would be to generate a summary table from a table of
// observations.
// When two or more rows of the input table share a value in the
// IndexColumn, the values from these rows will be combined on a
// column-by-column basis.  By default, such numerical values will be
// reduced to their mean, and non-numerical values will be reduced to
// their mode.  This default behavior can be changed by calling
// SetNumericalReductionMethod() or SetNonNumericalReductionMethod().
// You can also specify the reduction method to use for a particular
// column by calling SetReductionMethodForColumn().

#ifndef vtkReduceTable_h
#define vtkReduceTable_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkTableAlgorithm.h"

#include <map>                   // For ivar
#include <set>                   // For ivar
#include <vector>                // For ivar

class vtkVariant;

class VTKINFOVISCORE_EXPORT vtkReduceTable : public vtkTableAlgorithm
{
public:
  static vtkReduceTable* New();
  vtkTypeMacro(vtkReduceTable,vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the column that will be used to reduce the input table.
  // Any rows sharing a value in this column will be collapsed into
  // a single row in the output table.
  vtkGetMacro(IndexColumn, vtkIdType);
  vtkSetMacro(IndexColumn, vtkIdType);

  // Description:
  // Get/Set the method that should be used to combine numerical
  // values.
  vtkGetMacro(NumericalReductionMethod, int);
  vtkSetMacro(NumericalReductionMethod, int);

  // Description:
  // Get/Set the method that should be used to combine non-numerical
  // values.
  vtkGetMacro(NonNumericalReductionMethod, int);
  vtkSetMacro(NonNumericalReductionMethod, int);

  // Description:
  // Get the method that should be used to combine the values within
  // the specified column.  Returns -1 if no method has been set for
  // this particular column.
  int GetReductionMethodForColumn(vtkIdType col);

  // Description:
  // Set the method that should be used to combine the values within
  // the specified column.
  void SetReductionMethodForColumn(vtkIdType col, int method);

  //BTX
  // Description:
  // Enum for methods of reduction
  enum
    {
    MEAN,
    MEDIAN,
    MODE
    };
  //ETX

protected:
  vtkReduceTable();
  ~vtkReduceTable();

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

  // Description:
  // Initialize the output table to have the same types of columns as
  // the input table, but no rows.
  void InitializeOutputTable(vtkTable *input, vtkTable *output);

  // Description:
  // Find the distinct values in the input table's index column.
  // This function also populates the mapping of new row IDs to old row IDs.
  void AccumulateIndexValues(vtkTable *input);

  // Description:
  // Populate the index column of the output table.
  void PopulateIndexColumn(vtkTable *output);

  // Description:
  // Populate a non-index column of the output table.  This involves
  // potentially combining multiple values from the input table into
  // a single value for the output table.
  void PopulateDataColumn(vtkTable *input, vtkTable *output, vtkIdType col);

  // Description:
  // Find the mean of a series of values from the input table
  // and store it in the output table.
  void ReduceValuesToMean(vtkTable *input, vtkTable *output,
                          vtkIdType row, vtkIdType col,
                          std::vector<vtkIdType> oldRows);

  // Description:
  // Find the median of a series of values from the input table
  // and store it in the output table.
  void ReduceValuesToMedian(vtkTable *input, vtkTable *output,
                            vtkIdType row, vtkIdType col,
                            std::vector<vtkIdType> oldRows);

  // Description:
  // Find the mode of a series of values from the input table
  // and store it in the output table.
  void ReduceValuesToMode(vtkTable *input, vtkTable *output,
                          vtkIdType row, vtkIdType col,
                          std::vector<vtkIdType> oldRows);

  vtkIdType IndexColumn;
  std::set<vtkVariant> IndexValues;
  std::map<vtkVariant, std::vector<vtkIdType> > NewRowToOldRowsMap;
  std::map<vtkIdType, int> ColumnReductionMethods;

  int NumericalReductionMethod;
  int NonNumericalReductionMethod;

private:
  vtkReduceTable(const vtkReduceTable&); // Not implemented
  void operator=(const vtkReduceTable&);   // Not implemented
};

#endif
