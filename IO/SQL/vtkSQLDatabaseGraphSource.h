// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkSQLDatabaseGraphSource
 * @brief   Generates a vtkGraph based on an SQL query.
 *
 *
 * This class combines vtkSQLDatabase, vtkSQLQuery, and vtkQueryToGraph to
 * provide a convenience class for generating graphs from databases.
 * Also this class can be easily wrapped and used within ParaView / OverView.
 */

#ifndef vtkSQLDatabaseGraphSource_h
#define vtkSQLDatabaseGraphSource_h

#include "vtkGraphAlgorithm.h"
#include "vtkIOSQLModule.h" // For export macro
#include "vtkStdString.h"   // for vtkStdString

VTK_ABI_NAMESPACE_BEGIN
class vtkEventForwarderCommand;

class VTKIOSQL_EXPORT vtkSQLDatabaseGraphSource : public vtkGraphAlgorithm
{
public:
  static vtkSQLDatabaseGraphSource* New();
  vtkTypeMacro(vtkSQLDatabaseGraphSource, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkStdString GetURL();
  void SetURL(const vtkStdString& url);

  void SetPassword(const vtkStdString& password);

  vtkStdString GetEdgeQuery();
  void SetEdgeQuery(const vtkStdString& query);

  vtkStdString GetVertexQuery();
  void SetVertexQuery(const vtkStdString& query);

  void AddLinkVertex(const char* column, const char* domain = 0, int hidden = 0);
  void ClearLinkVertices();
  void AddLinkEdge(const char* column1, const char* column2);
  void ClearLinkEdges();

  ///@{
  /**
   * If on (default), generate edge pedigree ids.
   * If off, assign an array to be edge pedigree ids.
   */
  vtkGetMacro(GenerateEdgePedigreeIds, bool);
  vtkSetMacro(GenerateEdgePedigreeIds, bool);
  vtkBooleanMacro(GenerateEdgePedigreeIds, bool);
  ///@}

  ///@{
  /**
   * Use this array name for setting or generating edge pedigree ids.
   */
  vtkSetStringMacro(EdgePedigreeIdArrayName);
  vtkGetStringMacro(EdgePedigreeIdArrayName);
  ///@}

  ///@{
  /**
   * If on (default), generate a directed output graph.
   * If off, generate an undirected output graph.
   */
  vtkSetMacro(Directed, bool);
  vtkGetMacro(Directed, bool);
  vtkBooleanMacro(Directed, bool);
  ///@}

protected:
  vtkSQLDatabaseGraphSource();
  ~vtkSQLDatabaseGraphSource() override;

  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  bool GenerateEdgePedigreeIds;
  char* EdgePedigreeIdArrayName;

private:
  vtkSQLDatabaseGraphSource(const vtkSQLDatabaseGraphSource&) = delete;
  void operator=(const vtkSQLDatabaseGraphSource&) = delete;

  /**
   * This intercepts events from the graph layout class
   * and re-emits them as if they came from this class.
   */
  vtkEventForwarderCommand* EventForwarder;

  class implementation;
  implementation* const Implementation;

  bool Directed;
};

VTK_ABI_NAMESPACE_END
#endif
