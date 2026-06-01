//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_datamodel_ReadPlan_H_
#define fides_datamodel_ReadPlan_H_

#include <fides/DataSource.h>
#include <fides/FidesTypes.h>
#include <fides/MetaData.h>
#include <fides/internal/DataModel.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace fides
{
namespace datamodel
{

/// \brief Selection-relevant subset of \c fides::metadata::MetaData.
///
/// Two reads of the same variable produce the same data iff they share
/// the same \c SelectionKey. Fields included here are the ones that
/// flow through to ADIOS2 and change which bytes are returned: block
/// subset, step index, group name, and the multi-block-flatten flag.
/// Other \c MetaData entries (e.g. \c FIELDS, \c TIME_VALUE) don't
/// affect a single \c ReadVariable call's result and are deliberately
/// excluded.
struct SelectionKey
{
  std::vector<size_t> BlockSelection;
  bool HasBlockSelection = false;

  size_t StepSelection = 0;
  bool HasStepSelection = false;

  std::string GroupSelection;
  bool HasGroupSelection = false;

  /// Build a key by extracting the relevant fields from a MetaData
  /// snapshot. Missing fields produce a "no selection" key with the
  /// corresponding \c Has* flag false. The \c READ_AS_MULTIBLOCK flag
  /// is intentionally not part of the key — it's carried on
  /// \c ReadRequest::MultiBlock as the explicit dispatch knob.
  static SelectionKey From(const fides::metadata::MetaData& selections);

  bool operator==(const SelectionKey& other) const;
};

/// \brief Description of a single deferred ADIOS2 read.
///
/// Two requests are equal (and dedup-eligible) iff they refer to the
/// same data source, variable name, multi-block dispatch, vector
/// interpretation, and selection key. Equal requests share one
/// underlying \c DataSource::ReadVariable / \c ReadMultiBlockVariable
/// call inside \c ReadPlan::Execute.
struct ReadRequest
{
  std::string DataSourceName;
  std::string VariableName;
  fides::io::IsVector IsVector = fides::io::IsVector::Auto;
  /// Dispatch to \c ReadMultiBlockVariable instead of \c ReadVariable.
  bool MultiBlock = false;
  SelectionKey Selection;

  bool operator==(const ReadRequest& other) const;
};

struct ReadRequestHash
{
  size_t operator()(const ReadRequest& req) const;
};

/// Map from request to its result vector (one \c RawArray per block).
using ReadResultMap =
  std::unordered_map<ReadRequest, std::vector<fides::RawArray>, ReadRequestHash>;

/// \brief Coalesces a batch of \c ReadRequest objects into one
/// \c DataSource read per unique request, returning the result map.
///
/// Components participating in the plan-then-execute flow:
///   1. Append their requests via \c CollectReadRequests.
///   2. Receive the resulting \c ReadResultMap and look up their
///      \c RawArrays by request.
///
/// \c selections is the original \c MetaData passed to the top-level
/// \c Read; \c ReadPlan reconstructs the per-request \c MetaData from
/// \c SelectionKey + this snapshot for non-key fields (so e.g.
/// \c PLANE_SELECTION carries through even though it isn't part of
/// the dedup key).
class ReadPlan
{
public:
  static ReadResultMap Execute(const std::vector<ReadRequest>& requests,
                               const std::unordered_map<std::string, std::string>& paths,
                               DataSourcesType& sources,
                               const fides::metadata::MetaData& selections);
};

}
}

#endif
