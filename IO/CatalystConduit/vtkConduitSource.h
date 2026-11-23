// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkConduitSource
 * @brief data source for Conduit Mesh Blueprint.
 *
 * vtkConduitSource is a data source that processes a Conduit node
 * using [Conduit Mesh
 * Blueprint](https://llnl-conduit.readthedocs.io/en/latest/blueprint_mesh.html#)
 * to describe computational mesh and associated meta-data.
 *
 * vtkConduitSource currently produces a `vtkParitionedDataSet`,
 * `vtkPartitionedDataSetCollection` or `vtkOverlappingAMR` for meshes, and a `vtkTable`
 * (or a `vtkPartitionedDataSetCollection` of tables) for the Conduit table blueprint.
 *
 * @sa vtkConduitArrayUtilities
 */

#ifndef vtkConduitSource_h
#define vtkConduitSource_h

#include "vtkDataObjectAlgorithm.h"

#include "vtkIOCatalystConduitModule.h" // for exports

#include "conduit.h" // for conduit_node

#include <memory> // for std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class VTKIOCATALYSTCONDUIT_EXPORT vtkConduitSource : public vtkDataObjectAlgorithm
{
public:
  static vtkConduitSource* New();
  vtkTypeMacro(vtkConduitSource, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * vtkConduitSource supports single 'mesh' and multiple 'mesh' (aka 'multimesh')
   * as well as AMR mesh (aka 'amrmesh') protocols.
   * Set this to true if the source is handling amrmesh (default is false).
   */
  vtkSetMacro(UseAMRMeshProtocol, bool);
  vtkGetMacro(UseAMRMeshProtocol, bool);
  vtkBooleanMacro(UseAMRMeshProtocol, bool);
  ///@}

  ///@{
  /**
   * vtkConduitSource supports single 'mesh' and multiple 'mesh' (aka 'multimesh')
   * as well as AMR mesh (aka 'amrmesh') protocols.
   * Set this to true if the source is handling multimesh (default is false).
   */
  vtkSetMacro(UseMultiMeshProtocol, bool);
  vtkGetMacro(UseMultiMeshProtocol, bool);
  vtkBooleanMacro(UseMultiMeshProtocol, bool);
  ///@}

  ///@{
  /**
   * Set this to true if the node follows the Conduit table blueprint (default is false).
   * It takes precedence over UseAMRMeshProtocol and UseMultiMeshProtocol.
   *
   * A single table (a node with a "values" child) produces a vtkTable. An empty node, or one
   * holding only "state" and "fields", produces an empty vtkTable, so a rank with no rows, such
   * as one that does not hold a global quantity, can still pass its state.
   *
   * A node holding several named tables produces a vtkPartitionedDataSetCollection with one
   * table per partitioned dataset, named in its metadata, or a vtkMultiBlockDataSet when
   * OutputMultiBlock is on. "state" and "fields" children are not tables. Each table's
   * "state/fields" and "fields" become field data on its partitioned dataset, and the assembly
   * node applies as for meshes. Every rank must name the same tables; a child that is empty or
   * holds only "state" and "fields" gives an empty table on a rank that has no rows for it, and
   * any other child without "values" is an error. Ranks that produce different output types
   * fail on every rank.
   */
  vtkSetMacro(UseTableProtocol, bool);
  vtkGetMacro(UseTableProtocol, bool);
  vtkBooleanMacro(UseTableProtocol, bool);
  ///@}

  ///@{
  /**
   * vtkConduitSource supports output vtkMultiBlock instead of vtkPartitionedDataSetCollection
   * Set this to true if the source should output vtkMultiBlock (default is false).
   */
  vtkSetMacro(OutputMultiBlock, bool);
  vtkGetMacro(OutputMultiBlock, bool);
  vtkBooleanMacro(OutputMultiBlock, bool);
  ///@}

  ///@{
  /**
   * Get/Set the conduit_node. This must be the node satisfying the Conduit Mesh
   * Blueprint.
   */
  void SetNode(const conduit_node* node);
  ///@}

  ///@{
  /**
   * Mechanism to add global / field-data arrays.
   *
   * This is currently experimental and expected to change. It is experimental
   * since it's unclear to the developer if Conduit Blueprint already supports
   * specifying global fields i.e. without any association. Doesn't look like
   * it, but if it does, this should be changed to directly leverage that.
   */
  void SetGlobalFieldsNode(const conduit_node* node);
  ///@}

  ///@{
  /**
   * Set the node to read the assembly information from, if any.
   */
  void SetAssemblyNode(const conduit_node* node);
  ///@}

protected:
  vtkConduitSource();
  ~vtkConduitSource() override;

  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  ///@{
  /**
   * Generate actual output and fill the given data object with it.
   * Return true if data is correctly generated.
   */
  bool GenerateAMR(vtkDataObject* output);
  bool GeneratePartitionedDataSet(vtkDataObject* output);
  bool GeneratePartitionedDataSetCollection(vtkDataObject* output);
  bool GenerateTable(vtkDataObject* output);
  ///@}

private:
  vtkConduitSource(const vtkConduitSource&) = delete;
  void operator=(const vtkConduitSource&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
  bool UseAMRMeshProtocol;
  bool UseMultiMeshProtocol;
  bool UseTableProtocol;
  bool OutputMultiBlock;
};
VTK_ABI_NAMESPACE_END

#endif
