/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtSQLQuery.h

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
// .NAME vtkQtSQLQuery - query class associated with vtkQtSQLDatabase
//
// .SECTION Description
// Implements vtkSQLQuery using an underlying QSQLQuery.

#ifndef vtkQtSQLQuery_h
#define vtkQtSQLQuery_h

// Check for Qt SQL module before defining this class.
#include <qglobal.h>
#if (QT_EDITION & QT_MODULE_SQL)

#include "vtkGUISupportQtSQLModule.h"
#include "vtkSQLQuery.h"
#include "vtkType.h"

class vtkVariant;
class vtkQtSQLQueryInternals;

class VTKGUISUPPORTQTSQL_EXPORT vtkQtSQLQuery : public vtkSQLQuery
{
public:
  static vtkQtSQLQuery* New();
  vtkTypeMacro(vtkQtSQLQuery, vtkSQLQuery);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Execute the query.  This must be performed
  // before any field name or data access functions
  // are used.
  virtual bool Execute();

  // Description:
  // The number of fields in the query result.
  virtual int GetNumberOfFields();

  // Description:
  // Return the name of the specified query field.
  virtual const char* GetFieldName(int col);

  // Description:
  // Return the type of the specified query field, as defined in vtkType.h.
  virtual int GetFieldType(int col);

  // Description:
  // Advance row, return false if past end.
  virtual bool NextRow();

  // Description:
  // Return data in current row, field c
  virtual vtkVariant DataValue(vtkIdType c);

  // Description:
  // Returns true if an error is set, otherwise false.
  virtual bool HasError();

  // Description:
  // Get the last error text from the query
  virtual const char* GetLastErrorText();

protected:
  vtkQtSQLQuery();
  ~vtkQtSQLQuery();

  vtkQtSQLQueryInternals* Internals;
  friend class vtkQtSQLDatabase;

private:

  // Using the convenience function internally
  vtkSetStringMacro(LastErrorText);

  char* LastErrorText;

  vtkQtSQLQuery(const vtkQtSQLQuery &); // Not implemented.
  void operator=(const vtkQtSQLQuery &); // Not implemented.
};

#endif // (QT_EDITION & QT_MODULE_SQL)
#endif // vtkQtSQLQuery_h

