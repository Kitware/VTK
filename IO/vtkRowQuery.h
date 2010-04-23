/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRowQuery.h

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
// .NAME vtkRowQuery - abstract interface for queries that return
// row-oriented results.
//
// .SECTION Description
// The abstract superclass of query classes that return row-oriented (table)
// results.  A subclass will provide database-specific query parameters and
// implement the vtkRowQuery API to return query results:
//
// Execute() - Execute the query.  No results need to be retrieved at this
//             point, unless you are performing caching.
//
// GetNumberOfFields() - After Execute() is performed, returns the number
//                       of fields in the query results.
//
// GetFieldName() - The name of the field at an index.
//
// GetFieldType() - The data type of the field at an index.
//
// NextRow() - Advances the query results by one row, and returns whether
//             there are more rows left in the query.
//
// DataValue() - Extract a single data value from the current row.
//
// .SECTION Thanks
// Thanks to Andrew Wilson from Sandia National Laboratories for his work
// on the database classes.
//
// .SECTION See Also
// vtkRowQueryToTable

#ifndef __vtkRowQuery_h
#define __vtkRowQuery_h

#include "vtkObject.h"

class vtkVariant;
class vtkVariantArray;

class VTK_IO_EXPORT vtkRowQuery : public vtkObject
{
public:
  vtkTypeMacro(vtkRowQuery, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Execute the query.  This must be performed
  // before any field name or data access functions
  // are used.
  virtual bool Execute() = 0;

  // Description:
  // The number of fields in the query result.
  virtual int GetNumberOfFields() = 0;

  // Description:
  // Return the name of the specified query field.
  virtual const char* GetFieldName(int i) = 0;

  // Description:
  // Return the type of the field, using the constants defined in vtkType.h.
  virtual int GetFieldType(int i) = 0;

  // Description:
  // Return the index of the specified query field.
  // Uses GetNumberOfFields() and GetFieldName()
  // to match field name.
  int GetFieldIndex(char* name);

  // Description:
  // Advance row, return false if past end.
  virtual bool NextRow() = 0;

  // Description:
  // Return true if the query is active (i.e. execution was successful
  // and results are ready to be fetched).  Returns false on error or
  // inactive query.
  virtual bool IsActive() = 0;

//BTX
  // Description:
  // Advance row, return false if past end.
  // Also, fill array with row values.
  bool NextRow(vtkVariantArray* rowArray);

  // Description:
  // Return data in current row, field c
  virtual vtkVariant DataValue(vtkIdType c) = 0;
//ETX

  // Description:
  // Returns true if an error is set, otherwise false.
  virtual bool HasError() = 0;
  
  // Description:
  // Get the last error text from the query
  virtual const char* GetLastErrorText() = 0;

  // Description:
  // Many databases do not preserve case in field names.  This can
  // cause GetFieldIndex to fail if you search for a field named
  // someFieldName when the database actually stores it as
  // SOMEFIELDNAME.  This ivar controls whether GetFieldIndex()
  // expects field names to be case-sensitive.  The default is OFF,
  // i.e. case is not preserved.
  vtkSetMacro(CaseSensitiveFieldNames, bool);
  vtkGetMacro(CaseSensitiveFieldNames, bool);
  vtkBooleanMacro(CaseSensitiveFieldNames, bool);

protected:
  vtkRowQuery();
  ~vtkRowQuery();
  bool CaseSensitiveFieldNames;
private:
  vtkRowQuery(const vtkRowQuery &); // Not implemented.
  void operator=(const vtkRowQuery &); // Not implemented.
};

#endif // __vtkRowQuery_h

