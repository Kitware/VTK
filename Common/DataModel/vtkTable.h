/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
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

class vtkAbstractArray;
class vtkDataSetAttributes;
class vtkVariant;
class vtkVariantArray;

class VTKCOMMONDATAMODEL_EXPORT vtkTable : public vtkDataObject
{
public:
  static vtkTable* New();
  vtkTypeMacro(vtkTable, vtkDataObject);
  void PrintSelf(ostream &os, vtkIndent indent) override;

  /**
   * Dump table contents.  If rowLimit is -1 then the full table
   * is printed out (Default).  If rowLimit is 0 then only the
   * header row will be displayed.  Otherwise, if rowLimit > 0
   * then Dump will print the first rowLimit rows of data.
   */
  void Dump( unsigned int colWidth = 16, int rowLimit = -1 );

  /**
   * Return what type of dataset this is.
   */
  int GetDataObjectType() override {return VTK_TABLE;}

  /**
   * Return the actual size of the data in kibibytes (1024 bytes). This number
   * is valid only after the pipeline has updated. The memory size
   * returned is guaranteed to be greater than or equal to the
   * memory required to represent the data (e.g., extra space in
   * arrays, etc. are not included in the return value).
   */
  unsigned long GetActualMemorySize() override;

  //@{
  /**
   * Get/Set the main data (columns) of the table.
   */
  vtkGetObjectMacro(RowData, vtkDataSetAttributes);
  virtual void SetRowData(vtkDataSetAttributes* data);
  //@}

  //
  // Row functions
  //

  /**
   * Get the number of rows in the table.
   */
  vtkIdType GetNumberOfRows();

  /**
   * Set the number of rows in the table. Note that memory allocation might be performed
   * as a result of this, but no memory will be released.
   */
  void SetNumberOfRows(const vtkIdType );

  /**
   * Get a row of the table as a vtkVariantArray which has one entry for each column.
   * NOTE: This version of the method is NOT thread safe.
   */
  vtkVariantArray* GetRow(vtkIdType row);

  /**
   * Get a row of the table as a vtkVariantArray which has one entry for each column.
   */
  void GetRow(vtkIdType row, vtkVariantArray *values);

  /**
   * Set a row of the table with a vtkVariantArray which has one entry for each column.
   */
  void SetRow(vtkIdType row, vtkVariantArray *values);

  /**
   * Insert a blank row at the end of the table.
   */
  vtkIdType InsertNextBlankRow(double default_num_val=0.0);

  /**
   * Insert a row specified by a vtkVariantArray.  The number of entries in the array
   * should match the number of columns in the table.
   */
  vtkIdType InsertNextRow(vtkVariantArray* arr);

  /**
   * Delete a row from the table.  Rows below the deleted row are shifted up.
   */
  void RemoveRow(vtkIdType row);

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
   * Get a column of the table by its column index.
   */
  vtkAbstractArray* GetColumn(vtkIdType col);

  /**
   * Add a column to the table.
   */
  void AddColumn(vtkAbstractArray* arr);

  /**
   * Remove a column from the table by its name.
   */
  void RemoveColumnByName(const char* name);

  /**
   * Remove a column from the table by its column index.
   */
  void RemoveColumn(vtkIdType col);

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

  //@{
  /**
   * Retrieve the table from vtkInformation.
   */
  static vtkTable* GetData(vtkInformation* info);
  static vtkTable* GetData(vtkInformationVector* v, int i=0);
  //@}

  //@{
  /**
   * Shallow/deep copy the data from src into this object.
   */
  void ShallowCopy(vtkDataObject* src) override;
  void DeepCopy(vtkDataObject* src) override;
  //@}

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

private:
  vtkTable(const vtkTable&) = delete;
  void operator=(const vtkTable&) = delete;
};

#endif

