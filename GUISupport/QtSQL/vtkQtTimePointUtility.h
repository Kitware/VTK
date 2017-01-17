/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtTimePointUtility.h

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
 * @class   vtkQtTimePointUtility
 * @brief   performs common time operations
 *
 *
 * vtkQtTimePointUtility is provides methods to perform common time operations.
*/

#ifndef vtkQtTimePointUtility_h
#define vtkQtTimePointUtility_h

#include "vtkGUISupportQtSQLModule.h" // For export macro
#include "vtkObject.h"
#include <QDateTime> // Needed for method return types

class VTKGUISUPPORTQTSQL_EXPORT vtkQtTimePointUtility : public vtkObject
{
public:
  vtkTypeMacro(vtkQtTimePointUtility,vtkObject);

  static QDateTime TimePointToQDateTime(vtkTypeUInt64 time);
  static vtkTypeUInt64 QDateTimeToTimePoint(QDateTime time);
  static vtkTypeUInt64 QDateToTimePoint(QDate date);
  static vtkTypeUInt64 QTimeToTimePoint(QTime time);

protected:
  vtkQtTimePointUtility() {}
  ~vtkQtTimePointUtility() VTK_OVERRIDE {}

private:
  vtkQtTimePointUtility(const vtkQtTimePointUtility&) VTK_DELETE_FUNCTION;
  void operator=(const vtkQtTimePointUtility&) VTK_DELETE_FUNCTION;
};

#endif
