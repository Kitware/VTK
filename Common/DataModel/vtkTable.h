// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkTable
 * @brief   A table, which contains similar-typed columns of data
 *
 *
 * vtkTable is a basic data structure for storing columns of data.
 * Internally, columns are stored in a vtkDataSetAttributes structure called
 * RowData. However, using the vtkTable API additionally ensures that every column
 * has the same number of entries, and provides row access (using vtkVariantArray)
 * and single entry access (using vtkVariant).
 *
 * Inserting or removing rows via the class API preserves existing table data where possible.
 *
 * The "RemoveRow*" and SetNumberOfRows() operations will not release memory. Call on SqueezeRows()
 * to achieve this after performing the operations.
 *
 * The field data inherited from vtkDataObject may be used to store metadata
 * related to the table.
 *
 * @warning
 * You should use the vtkTable API to change the table data. Performing
 * operations on the object returned by GetRowData() may
 * yield unexpected results. vtkTable does allow the user to set the field
 * data using SetRowData(); the number of rows in the table is determined
 * by the number of tuples in the first array (it is assumed that all arrays
 * are the same length).
 *
 * @warning
 * Each column added with AddColumn <b>must</b> have its name set to a unique,
 * non-empty string in order for GetValue() to function properly.
 *
 * @par Thanks:
 * Thanks to Patricia Crossno, Ken Moreland, Andrew Wilson and Brian Wylie from
 * Sandia National Laboratories for their help in developing this class API.
 */

#ifndef vtkTable_h
#define vtkTable_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataObject.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALMANUAL

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;
class vtkDataSetAttributes;
class vtkVariant;
class vtkVariantArray;

class VTKCOMMONDATAMODEL_EXPORT VTK_MARSHALMANUAL vtkTable : public vtkDataObject
{
public:
  static vtkTable* New();
  static vtkTable* ExtendedNew();
  vtkTypeMacro(vtkTable, vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Dump table contents.  If rowLimit is -1 then the full table
   * is printed out (Default).  If rowLimit is 0 then only the
   * header row will be displayed.  Otherwise, if rowLimit > 0
   * then Dump will print the first rowLimit rows of data.
   */
  void Dump(unsigned int colWidth = 16, int rowLimit = -1);

  /**
   * Return what type of dataset this is.
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_TABLE; }

  /**
   * Return the actual size of the data in kibibytes (1024 bytes). This number
   * is valid only after the pipeline has updated. The memory size
   * returned is guaranteed to be greater than or equal to the
   * memory required to represent the data (e.g., extra space in
   * arrays, etc. are not included in the return value).
   */
  unsigned long GetActualMemorySize() override;

  ///@{
  /**
   * Get/Set the main data (columns) of the table.
   */
  vtkGetObjectMacro(RowData, vtkDataSetAttributes);
  virtual void SetRowData(vtkDataSetAttributes* data);
  ///@}

  //
  // Row functions
  //

  /**
   * Get the number of rows in the table.
   */
  vtkIdType GetNumberOfRows();

  /**
   * Set the number of rows in the table. Note that memory allocation might be performed
   * as a result of this, but no memory will be released. Existing data is preserved if the table is
   * expanding.
   */
  void SetNumberOfRows(vtkIdType);

  /**
   * Release previously allocated and now unused memory after performing resizing operations.
   */
  void SqueezeRows();

  /**
   * Get a row of the table as a vtkVariantArray which has one entry for each column.
   * NOTE: This version of the method is NOT thread safe.
   */
  vtkVariantArray* GetRow(vtkIdType row);

  /**
   * Get a row of the table as a vtkVariantArray which has one entry for each column.
   */
  void GetRow(vtkIdType row, vtkVariantArray* values);

  /**
   * Set a row of the table with a vtkVariantArray which has one entry for each column.
   */
  void SetRow(vtkIdType row, vtkVariantArray* values);

  /**
   * Insert a single row at the index.
   */
  void InsertRow(vtkIdType row);

  /**
   * Insert n rows before row. If row < 0 then the rows will be prepended to the table.
   */
  void InsertRows(vtkIdType row, vtkIdType n);

  /**
   * Insert a blank row at the end of the table.
   */
  vtkIdType InsertNextBlankRow(double default_num_val = 0.0);

  /**
   * Insert a row at the end of the tablespecified by a vtkVariantArray. The number of entries in
   * the array should match the number of columns in the table.
   */
  vtkIdType InsertNextRow(vtkVariantArray* values);

  /**
   * Delete a single row from the table. Rows below the deleted row are shifted up.
   */
  void RemoveRow(vtkIdType row);

  /**
   * Delete n rows from the table, starting at row. Rows below the deleted rows are shifted up.
   */
  void RemoveRows(vtkIdType row, vtkIdType n);

  /**
   * Delete all rows from the table. The column arrays are not delete, they are just empty
   * after this operation.
   */
  void RemoveAllRows();

  //
  // Column functions
  //

  /**
   * Get the number of columns in the table.
   */
  vtkIdType GetNumberOfColumns();

  // Get the name of a column of the table.
  const char* GetColumnName(vtkIdType col);

  /**
   * Get a column of the table by its name.
   */
  vtkAbstractArray* GetColumnByName(const char* name);

  /**
   * Get the column index for a name.
   * If name is not found returns -1.
   */
  vtkIdType GetColumnIndex(const char* name);

  /**
   * Get a column of the table by its column index.
   */
  vtkAbstractArray* GetColumn(vtkIdType col);

  /**
   * Add a column to the table.
   */
  void AddColumn(vtkAbstractArray* arr);

  /**
   * Insert a column into the table at given column index.
   */
  void InsertColumn(vtkAbstractArray* arr, vtkIdType index);

  /**
   * Remove a column from the table by its name.
   */
  void RemoveColumnByName(const char* name);

  /**
   * Remove a column from the table by its column index.
   */
  void RemoveColumn(vtkIdType col);

  /**
   * Remove all columns from the table.
   */
  void RemoveAllColumns();

  //
  // Table single entry functions
  //

  /**
   * Retrieve a value in the table by row and column index as a variant.
   * Note that this calls GetValueByName internally so that each column
   * array must have its name set (and that name should be unique within
   * the table).
   */
  vtkVariant GetValue(vtkIdType row, vtkIdType col);

  /**
   * Retrieve a value in the table by row index and column name as a variant.
   */
  vtkVariant GetValueByName(vtkIdType row, const char* col);

  /**
   * Set a value in the table by row and column index as a variant.
   */
  void SetValue(vtkIdType row, vtkIdType col, vtkVariant value);

  /**
   * Set a value in the table by row index and column name as a variant.
   */
  void SetValueByName(vtkIdType row, const char* col, vtkVariant value);

  /**
   * Initialize to an empty table.
   */
  void Initialize() override;

  ///@{
  /**
   * Retrieve the table from vtkInformation.
   */
  static vtkTable* GetData(vtkInformation* info);
  static vtkTable* GetData(vtkInformationVector* v, int i = 0);
  ///@}

  ///@{
  /**
   * Shallow/deep copy the data from src into this object.
   */
  void ShallowCopy(vtkDataObject* src) override;
  void DeepCopy(vtkDataObject* src) override;
  ///@}

  /**
   * Returns the attributes of the data object as a vtkFieldData.
   * This returns non-null values in all the same cases as GetAttributes,
   * in addition to the case of FIELD, which will return the field data
   * for any vtkDataObject subclass.
   */
  vtkFieldData* GetAttributesAsFieldData(int type) override;

  /**
   * Get the number of elements for a specific attribute type (ROW, etc.).
   */
  vtkIdType GetNumberOfElements(int type) override;

protected:
  vtkTable();
  ~vtkTable() override;

  /**
   * Holds the column data of the table.
   */
  vtkDataSetAttributes* RowData;

  /**
   * Holds row information returned by GetRow().
   */
  vtkVariantArray* RowArray;

  /**
   * Move the content of the rows, starting first row and including last row. The rows
   * will be moved by delta, which can be positive or negative. No checks will be performed that the
   * arrays are correctly sized.
   */
  void MoveRowData(vtkIdType first, vtkIdType last, vtkIdType delta);

private:
  vtkTable(const vtkTable&) = delete;
  void operator=(const vtkTable&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
