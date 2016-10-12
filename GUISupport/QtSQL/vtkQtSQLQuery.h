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
/**
 * @class   vtkQtSQLQuery
 * @brief   query class associated with vtkQtSQLDatabase
 *
 *
 * Implements vtkSQLQuery using an underlying QSQLQuery.
*/

#ifndef vtkQtSQLQuery_h
#define vtkQtSQLQuery_h

// Check for Qt SQL module before defining this class.
#include <qglobal.h> // Needed to check if SQL is available
#if (QT_EDITION & QT_MODULE_SQL)

#include "vtkGUISupportQtSQLModule.h" // For export macro
#include "vtkSQLQuery.h"

class vtkVariant;
class vtkQtSQLQueryInternals;

class VTKGUISUPPORTQTSQL_EXPORT vtkQtSQLQuery : public vtkSQLQuery
{
public:
  static vtkQtSQLQuery* New();
  vtkTypeMacro(vtkQtSQLQuery, vtkSQLQuery);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Execute the query.  This must be performed
   * before any field name or data access functions
   * are used.
   */
  virtual bool Execute();

  /**
   * The number of fields in the query result.
   */
  virtual int GetNumberOfFields();

  /**
   * Return the name of the specified query field.
   */
  virtual const char* GetFieldName(int col);

  /**
   * Return the type of the specified query field, as defined in vtkType.h.
   */
  virtual int GetFieldType(int col);

  /**
   * Advance row, return false if past end.
   */
  virtual bool NextRow();

  /**
   * Return data in current row, field c
   */
  virtual vtkVariant DataValue(vtkIdType c);

  /**
   * Returns true if an error is set, otherwise false.
   */
  virtual bool HasError();

  /**
   * Get the last error text from the query
   */
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

  vtkQtSQLQuery(const vtkQtSQLQuery &) VTK_DELETE_FUNCTION;
  void operator=(const vtkQtSQLQuery &) VTK_DELETE_FUNCTION;
};

#endif // (QT_EDITION & QT_MODULE_SQL)
#endif // vtkQtSQLQuery_h

