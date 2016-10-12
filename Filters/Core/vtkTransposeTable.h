/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransposeTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkTransposeTable
 * @brief   Transpose an input table.
 *
 *
 * This algorithm allows to transpose a vtkTable as a matrix.
 * Columns become rows and vice versa. A new column can be added to
 * the result table at index 0 to collect the name of the initial
 * columns (when AddIdColumn is true). Such a column can be used
 * to name the columns of the result.
 * Note that columns of the output table will have a variant type
 * is the columns of the initial table are not consistant.
*/

#ifndef vtkTransposeTable_h
#define vtkTransposeTable_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkTableAlgorithm.h"

class VTKFILTERSCORE_EXPORT vtkTransposeTable : public vtkTableAlgorithm
{
public:
  static vtkTransposeTable* New();
  vtkTypeMacro(vtkTransposeTable, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * This flag indicates if a column must be inserted at index 0
   * with the names (ids) of the input columns.
   * Default: true
   */
  vtkGetMacro(AddIdColumn, bool);
  vtkSetMacro(AddIdColumn, bool);
  vtkBooleanMacro(AddIdColumn, bool);
  //@}

  //@{
  /**
   * This flag indicates if the output column must be named using the
   * names listed in the index 0 column.
   * Default: false
   */
  vtkGetMacro(UseIdColumn, bool);
  vtkSetMacro(UseIdColumn, bool);
  vtkBooleanMacro(UseIdColumn, bool);
  //@}

  //@{
  /**
   * Get/Set the name of the id column added by option AddIdColumn.
   * Default: ColName
   */
  vtkGetStringMacro(IdColumnName);
  vtkSetStringMacro(IdColumnName);
  //@}

protected:
  vtkTransposeTable();
  ~vtkTransposeTable() VTK_OVERRIDE;

  int RequestData(vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) VTK_OVERRIDE;

  bool AddIdColumn;
  bool UseIdColumn;
  char* IdColumnName;

private:
  vtkTransposeTable(const vtkTransposeTable&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTransposeTable&) VTK_DELETE_FUNCTION;
};

#endif
