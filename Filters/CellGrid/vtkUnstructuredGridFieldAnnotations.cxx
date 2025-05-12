// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkUnstructuredGridFieldAnnotations.h"

#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataAssembly.h"
#include "vtkFiltersCellGrid.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStringArray.h"
#include "vtkUnstructuredGrid.h"

#include <sstream>

// Define this for debugging output:
#undef VTK_DBG_UGRID_TO_CGRID

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

namespace
{

// Escape a block name into a safe XPath query string
std::string escape(const std::string& name)
{
  return name;
}

// Split \a inString into substrings wherever \a delimiter occurs.
std::vector<std::string> split(const std::string& inString, const std::string& delimiter)
{
  std::vector<std::string> subStrings;
  std::size_t sIdx = 0;
  std::size_t eIdx = 0;
  while ((eIdx = inString.find(delimiter, sIdx)) < inString.size())
  {
    subStrings.emplace_back(inString.substr(sIdx, eIdx - sIdx));
    sIdx = eIdx + delimiter.size();
  }
  if (sIdx < inString.size())
  {
    subStrings.emplace_back(inString.substr(sIdx));
  }
  return subStrings;
}

bool testGlomSuffixes(const std::string& prefix, const std::string& suffix,
  const vtkUnstructuredGridFieldAnnotations::BlockAttributesValue& blockData,
  std::vector<vtkStringToken>& glommedNames, const std::set<std::string>& requiredSuffixes)
{
  // std::cout << "    glom " << prefix << "//" << suffix << " ";
  std::set<std::string> suffixSet(requiredSuffixes.begin(), requiredSuffixes.end());
  if (suffixSet.find(suffix) == suffixSet.end())
  {
    // std::cout << "NO\n";
    return false;
  }
  // We match the test field; use it to generate matching names and see which are present.
  for (const auto& testSuffix : requiredSuffixes)
  {
    vtkStringToken testName(prefix + testSuffix);
    if (blockData.FieldNames.find(testName) == blockData.FieldNames.end())
    {
      // std::cout << "no\n";
      return false;
    }
    glommedNames.push_back(testName);
  }
  // std::cout << "yes\n";
  return true;
}

bool testGlom(vtkStringToken fieldName,
  const vtkUnstructuredGridFieldAnnotations::BlockAttributesValue& blockData, vtkStringToken& glom,
  std::vector<vtkStringToken>& glommedNames)
{
  // clang-format off
  std::string testPrefix2 = fieldName.Data().substr(0, fieldName.Data().size() - 2);
  std::string testSuffix2 = fieldName.Data().substr(fieldName.Data().size() - 2, std::string::npos);
  std::string testPrefix1 = fieldName.Data().substr(0, fieldName.Data().size() - 1);
  std::string testSuffix1 = fieldName.Data().substr(fieldName.Data().size() - 1, std::string::npos);
  // First, test for tensor component names (full, symmetric; 3d then 2d),
  // then for vector component names (3d then 2d). The order is important.
  // We test upper- and lower-case variants separately to ensure any match
  // has consistent character case.
  if (
    testGlomSuffixes(testPrefix2, testSuffix2, blockData, glommedNames, {"XX", "XY", "XZ", "YX", "YY", "YZ", "ZX", "ZY", "ZZ"}) ||
    testGlomSuffixes(testPrefix2, testSuffix2, blockData, glommedNames, {"xx", "xy", "xz", "yx", "yy", "yz", "zx", "zy", "zz"}) ||
    testGlomSuffixes(testPrefix2, testSuffix2, blockData, glommedNames, {"XX", "XY", "XZ", "YY", "YZ", "ZZ"}) ||
    testGlomSuffixes(testPrefix2, testSuffix2, blockData, glommedNames, {"xx", "xy", "xz", "yy", "yz", "zz"}) ||
    testGlomSuffixes(testPrefix2, testSuffix2, blockData, glommedNames, {"XX", "XY", "YX", "YY"}) ||
    testGlomSuffixes(testPrefix2, testSuffix2, blockData, glommedNames, {"xx", "xy", "yx", "yy"}) ||
    testGlomSuffixes(testPrefix2, testSuffix2, blockData, glommedNames, {"XX", "XY", "YY"}) ||
    testGlomSuffixes(testPrefix2, testSuffix2, blockData, glommedNames, {"xx", "xy", "yy"}))
  {
    glom = testPrefix2;
    return true;
  }
  if (
    testGlomSuffixes(testPrefix1, testSuffix1, blockData, glommedNames, {"X", "Y", "Z"}) ||
    testGlomSuffixes(testPrefix1, testSuffix1, blockData, glommedNames, {"x", "y", "z"}) ||
    testGlomSuffixes(testPrefix1, testSuffix1, blockData, glommedNames, {"X", "Y"}) ||
    testGlomSuffixes(testPrefix1, testSuffix1, blockData, glommedNames, {"x", "y"}))
  {
    glom = testPrefix1;
    return true;
  }
  // clang-format on
  glommedNames.clear();
  return false;
}

} // anonymous namespace

vtkStandardNewMacro(vtkUnstructuredGridFieldAnnotations);

vtkUnstructuredGridFieldAnnotations::vtkUnstructuredGridFieldAnnotations() = default;

void vtkUnstructuredGridFieldAnnotations::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Data: " << this->Data.size() << "\n";
}

void vtkUnstructuredGridFieldAnnotations::FetchAnnotations(
  vtkFieldData* fieldData, vtkDataAssembly* assembly)
{
  if (!fieldData || !assembly)
  {
    return;
  }
  static constexpr char iossAnnotations[] = "Information Records";
  auto* infoRecords = vtkStringArray::SafeDownCast(fieldData->GetAbstractArray(iossAnnotations));
  if (!infoRecords)
  {
    return;
  }

  std::unordered_set<std::string> elementBlockNames;
  for (vtkIdType i = 0; i < infoRecords->GetNumberOfValues(); ++i)
  {
    const auto& record = infoRecords->GetValue(i);
    const std::vector<std::string> data = ::split(record, "::");
    // Examples:
    // "HDIV::eblock-0_0_0::CG::basis::Intrepid2_HDIV_HEX_I1_FEM"
    // "HGRAD::eblock-0_0::DG::basis::Intrepid2_HGRAD_QUAD_C2_FEM"
    // "HCURL::eblock-0_0_0::CG::basis::Intrepid2_HCURL_HEX_I1_FEM"
    // "HCURL::eblock-0_0_0::CG::field::E_Field"
    if (data.size() != 5)
    {
      continue;
    }

    // TODO: Test whether function space is supported.

    // Find the datasets mentioned in the label.
    // Currently, we only parse annotations for element blocks.
    std::vector<std::string> queries;
    queries.push_back("/IOSS/element_blocks/*[@label=\"" + escape(data[1]) + "\"]");
    auto nodeIds = assembly->SelectNodes(queries);
    if (nodeIds.empty())
    {
      vtkWarningMacro("Unmatched block \"" << escape(data[1]) << "\" in \"" << record << "\".");
      continue;
    }
    for (const auto& nodeId : nodeIds)
    {
      auto dataIds = assembly->GetDataSetIndices(nodeId, true);
      // NB: It is not an error for dataIds to be empty. (In that case, we presume the data
      //     is distributed and other ranks may have non-empty partitions of the node.)
      for (const auto& dataId : dataIds)
      {
        auto& blockData = this->Data[dataId];
        vtkStringToken recordType(data[3]);
        vtkStringToken recordFunctionSpace(data[0]);
        vtkStringToken recordDOFSharing(data[2]);
        BlockAttributesKey key{ recordDOFSharing, recordFunctionSpace };
        auto& blockRecord(blockData[key]);
        blockRecord.NodeIds.insert(nodeId);
        // Insert this record into the metadata for the corresponding dataset.
        switch (recordType.GetId())
        {
          case "field"_hash:
          {
            vtkStringToken fieldName(data[4]);
            blockRecord.FieldNames.insert(fieldName);
          }
          break;
          case "basis"_hash:
          {
            auto descriptor = ::split(data[4], "_");
            if (descriptor.size() == 5)
            {
              if (recordFunctionSpace != descriptor[1])
              {
                vtkWarningMacro("Function space of record ("
                  << recordFunctionSpace.Data() << ") and basis spec (" << descriptor[1]
                  << ") in \"" << record << "\" do not match. Skipping.");
                continue;
              }
              blockRecord.BasisSource = descriptor[0];
              blockRecord.FunctionSpace = descriptor[1];
              blockRecord.Shape = descriptor[2];
              blockRecord.QuadratureScheme = descriptor[3];
              blockRecord.Formulation = descriptor[4];
            }
            else
            {
              vtkWarningMacro("Basis record \"" << record << "\" malformed. Skipping.");
            }
          }
          break;
          default:
            break; // Do nothing.
        }
      }
    }
  }

  // Now, for each block, attempt to glom fields together into
  // multi-component arrays.
  for (const auto& annotation : this->Data)
  {
    for (const auto& blockData : annotation.second)
    {
      std::vector<vtkStringToken> glommedNames;
      std::set<vtkStringToken> fieldNamesToRemove;
      for (const auto& fieldName : blockData.second.FieldNames)
      {
        if (fieldNamesToRemove.find(fieldName) != fieldNamesToRemove.end())
        {
          continue; // Skip this name; it is already glommed.
        }
        vtkStringToken glom;
        if (::testGlom(fieldName, blockData.second, glom, glommedNames))
        {
          blockData.second.FieldGloms[glom].Members = glommedNames;
          fieldNamesToRemove.insert(glommedNames.begin(), glommedNames.end());
        }
      }
      for (const auto& entry : fieldNamesToRemove)
      {
        blockData.second.FieldNames.erase(entry);
      }
    }
  }

#ifdef VTK_DBG_UGRID_TO_CGRID
  for (const auto& blockEntry : this->Data)
  {
    std::cout << "Block " << blockEntry.first << ":\n";
    for (const auto& keyValue : blockEntry.second)
    {
      std::cout << "  " << keyValue.first.FunctionSpace.Data() << "  "
                << keyValue.first.DOFSharing.Data() << "  "
                << keyValue.second.QuadratureScheme.Data() << "  " << keyValue.second.Shape.Data()
                << " " << keyValue.second.BasisSource.Data() << "  "
                << keyValue.second.Formulation.Data() << "\n";
      for (const auto& nodeId : keyValue.second.NodeIds)
      {
        std::cout << "    " << blockEntry.first << " " << nodeId << " = "
                  << assembly->GetNodePath(blockEntry.first) << "\n";
      }
      for (const auto& fieldName : keyValue.second.FieldNames)
      {
        std::cout << "    \"" << fieldName.Data() << "\"\n";
      }
      for (const auto& glomEntry : keyValue.second.FieldGloms)
      {
        std::cout << "    \"" << glomEntry.first.Data() << "\"\n";
        for (const auto& glomName : glomEntry.second.Members)
        {
          std::cout << "      \"" << glomName.Data() << "\"\n";
        }
      }
    }
  }
#endif // VTK_DBG_UGRID_TO_CGRID
}

void vtkUnstructuredGridFieldAnnotations::AddAnnotations(
  vtkFieldData* fieldData, vtkDataAssembly* assembly)
{
  (void)fieldData;
  (void)assembly;
  // TODO.
}

void vtkUnstructuredGridFieldAnnotations::Reset()
{
  this->Data.clear();
}

VTK_ABI_NAMESPACE_END
