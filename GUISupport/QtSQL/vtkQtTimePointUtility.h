// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
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

VTK_ABI_NAMESPACE_BEGIN
class VTKGUISUPPORTQTSQL_EXPORT vtkQtTimePointUtility : public vtkObject
{
public:
  vtkTypeMacro(vtkQtTimePointUtility, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static QDateTime TimePointToQDateTime(vtkTypeUInt64 time);
  static vtkTypeUInt64 QDateTimeToTimePoint(QDateTime time);
  static vtkTypeUInt64 QDateToTimePoint(QDate date);
  static vtkTypeUInt64 QTimeToTimePoint(QTime time);

protected:
  vtkQtTimePointUtility() = default;
  ~vtkQtTimePointUtility() override = default;

private:
  vtkQtTimePointUtility(const vtkQtTimePointUtility&) = delete;
  void operator=(const vtkQtTimePointUtility&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
