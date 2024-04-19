// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkSQLDatabaseTableSource
 * @brief   Generates a vtkTable based on an SQL query.
 *
 *
 * This class combines vtkSQLDatabase, vtkSQLQuery, and vtkQueryToTable to
 * provide a convenience class for generating tables from databases.
 * Also this class can be easily wrapped and used within ParaView / OverView.
 */

#ifndef vtkSQLDatabaseTableSource_h
#define vtkSQLDatabaseTableSource_h

#include "vtkIOSQLModule.h" // For export macro
#include "vtkStdString.h"   // for vtkStdString
#include "vtkTableAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkEventForwarderCommand;

class VTKIOSQL_EXPORT vtkSQLDatabaseTableSource : public vtkTableAlgorithm
{
public:
  static vtkSQLDatabaseTableSource* New();
  vtkTypeMacro(vtkSQLDatabaseTableSource, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkStdString GetURL();
  void SetURL(const vtkStdString& url);

  void SetPassword(const vtkStdString& password);

  vtkStdString GetQuery();
  void SetQuery(const vtkStdString& query);

  ///@{
  /**
   * The name of the array for generating or assigning pedigree ids
   * (default "id").
   */
  vtkSetStringMacro(PedigreeIdArrayName);
  vtkGetStringMacro(PedigreeIdArrayName);
  ///@}

  ///@{
  /**
   * If on (default), generates pedigree ids automatically.
   * If off, assign one of the arrays to be the pedigree id.
   */
  vtkSetMacro(GeneratePedigreeIds, bool);
  vtkGetMacro(GeneratePedigreeIds, bool);
  vtkBooleanMacro(GeneratePedigreeIds, bool);
  ///@}

protected:
  vtkSQLDatabaseTableSource();
  ~vtkSQLDatabaseTableSource() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkSQLDatabaseTableSource(const vtkSQLDatabaseTableSource&) = delete;
  void operator=(const vtkSQLDatabaseTableSource&) = delete;

  char* PedigreeIdArrayName;
  bool GeneratePedigreeIds;

  /**
   * This intercepts events from the graph layout class
   * and re-emits them as if they came from this class.
   */
  vtkEventForwarderCommand* EventForwarder;

  class implementation;
  implementation* const Implementation;
};

VTK_ABI_NAMESPACE_END
#endif
