// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkUnstructuredGridToCellGrid.h"

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

#define VTK_DBG_UGRID_TO_CGRID 1

#define VTK_TRANSCRIBE_CELLGRID_PHASE_CLAIM 0
#define VTK_TRANSCRIBE_CELLGRID_PHASE_CONVERT 1

VTK_ABI_NAMESPACE_BEGIN

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
  const vtkUnstructuredGridToCellGrid::TranscribeQuery::BlockAttributesValue& blockData,
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
  const vtkUnstructuredGridToCellGrid::TranscribeQuery::BlockAttributesValue& blockData,
  vtkStringToken& glom, std::vector<vtkStringToken>& glommedNames)
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

using namespace vtk::literals; // for ""_token

vtkStandardNewMacro(vtkUnstructuredGridToCellGrid);
vtkStandardNewMacro(vtkUnstructuredGridToCellGrid::TranscribeQuery);

bool vtkUnstructuredGridToCellGrid::TranscribeQuery::Initialize()
{
  bool ok = this->Superclass::Initialize();
  if (!this->Input)
  {
    return false;
  }
  switch (this->Phase)
  {
    case VTK_TRANSCRIBE_CELLGRID_PHASE_CLAIM:
    {
      // Reset all claims (erasing the number of cells, but preserving any
      // preferred cell-type and priority value.
      for (auto& entry : this->CellTypeMap)
      {
        entry.second.NumberOfCells =
          -1; // TODO: If the input dataset has not been modified, do nothing?
      }
      // Populate the CellTypeMap with numbers of cells of each cell type present.
      auto it = vtk::TakeSmartPointer(this->Input->NewCellIterator());
      for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
      {
        int cellType = it->GetCellType();
        ++(this->CellTypeMap[cellType].NumberOfCells);
      }
    }
    break;
    case VTK_TRANSCRIBE_CELLGRID_PHASE_CONVERT:
      break;
  }
  return ok;
}

bool vtkUnstructuredGridToCellGrid::TranscribeQuery::Finalize()
{
  switch (this->Phase)
  {
    case VTK_TRANSCRIBE_CELLGRID_PHASE_CLAIM:
    {
      // TODO: Identify whether any cell types are unclaimed and warn or fail as configured.

      // Create cell-attribute instances on output to match input point- and cell-data arrays.
      // Also, create a "shape" cell-attribute instance for the geometry.
      vtkNew<vtkCellAttribute> shape;
      // NB: These values are hardwired for now. In the future, we should examine the
      //     claimed cell types and choose something appropriate.
      shape->Initialize("shape", "ℝ³", 3);
      this->Output->SetShapeAttribute(shape);
      this->Coordinates = this->Input->GetPoints()->GetData();
      if (this->Coordinates)
      {
        if (!this->Coordinates->GetName() || !this->Coordinates->GetName()[0])
        {
          this->Coordinates->SetName("points");
        }
        this->Output->GetAttributes("coordinates"_token)->SetVectors(this->Coordinates);
      }
    }
    break;
    case VTK_TRANSCRIBE_CELLGRID_PHASE_CONVERT:
      this->Input = nullptr;
      this->Output = nullptr;
      break;
  }
  return true;
}

bool vtkUnstructuredGridToCellGrid::TranscribeQuery::SumOutputCounts()
{
  bool allCellsClaimed = true;
  this->OutputAllocations.clear();
  for (const auto& entry : this->CellTypeMap)
  {
    if (entry.second.NumberOfCells > 0)
    {
      if (entry.second.CellType.IsValid())
      {
        this->OutputAllocations[entry.second.CellType] += entry.second.NumberOfCells;
        // clang-format off
        vtkLogF(TRACE, "Entry %zu += %lld for '%s' (%x)",
          this->OutputAllocations.size(),
          static_cast<long long>(entry.second.NumberOfCells),
          entry.second.CellType.Data().c_str(),
          entry.second.CellType.GetId());
        // clang-format on
      }
      else
      {
        // clang-format off
        vtkLogF(INFO, "No allocations for %llu cells of type %lu",
          static_cast<unsigned long long>(entry.second.NumberOfCells),
          static_cast<unsigned long>(entry.first));
        // clang-format on
        allCellsClaimed = false;
      }
    }
  }
  vtkLogF(TRACE, "%zu types with allocations", this->OutputAllocations.size());
  return allCellsClaimed;
}

void vtkUnstructuredGridToCellGrid::TranscribeQuery::AddCellAttributes(
  vtkDataSetAttributes* attributes)
{
  int numberOfArrays = attributes->GetNumberOfArrays();
  for (int aa = 0; aa < numberOfArrays; ++aa)
  {
    auto* arrayIn = attributes->GetAbstractArray(aa);
    if (!arrayIn || !arrayIn->GetName())
    {
      vtkWarningMacro("Empty or unnamed array " << aa << ".");
      continue;
    }
    int nc = arrayIn->GetNumberOfComponents();
    vtkNew<vtkCellAttribute> attribOut;
    std::string fieldSpace = vtkCellAttribute::EncodeSpace("ℝ", nc);
    attribOut->Initialize(arrayIn->GetName(), fieldSpace, nc);
    this->Output->AddCellAttribute(attribOut);
  }
}

vtkUnstructuredGridToCellGrid::vtkUnstructuredGridToCellGrid()
{
  vtkFiltersCellGrid::RegisterCellsAndResponders();
}

void vtkUnstructuredGridToCellGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkUnstructuredGridToCellGrid::Reset()
{
  // this->Request->CellTypeMap.clear();
  // this->Request->OutputAllocations.clear();
}

void vtkUnstructuredGridToCellGrid::AddPreferredOutputType(
  int inputCellType, vtkStringToken preferredOutputType, int priority)
{
  this->Request->CellTypeMap[inputCellType] =
    TranscribeQuery::Claim{ 0, priority, preferredOutputType };
}

int vtkUnstructuredGridToCellGrid::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port != 0)
  {
    return 0;
  }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
  return 1;
}

int vtkUnstructuredGridToCellGrid::RequestData(
  vtkInformation* vtkNotUsed(request), vtkInformationVector** inInfo, vtkInformationVector* ouInfo)
{
  vtkSmartPointer<vtkPartitionedDataSetCollection> inputPDC;
  inputPDC = vtkPartitionedDataSetCollection::GetData(inInfo[0]);
  if (!inputPDC)
  {
    auto* input = vtkUnstructuredGrid::GetData(inInfo[0]);
    if (input)
    {
      inputPDC = vtkSmartPointer<vtkPartitionedDataSetCollection>::New();
      inputPDC->SetNumberOfPartitionedDataSets(1);
      inputPDC->SetPartition(0, 0, input);
    }
  }
  auto* output = vtkPartitionedDataSetCollection::GetData(ouInfo);
  if (!inputPDC)
  {
    vtkWarningMacro("Empty input or input of wrong type.");
    return 1;
  }
  if (!output)
  {
    vtkErrorMacro("Empty output.");
    return 0;
  }

  // Copy the input's hierarchical block arrangement if it exists:
  if (inputPDC->GetDataAssembly())
  {
    vtkNew<vtkDataAssembly> dataAssembly;
    dataAssembly->DeepCopy(inputPDC->GetDataAssembly());
    output->SetDataAssembly(dataAssembly);
  }

  // Look for annotations specifying DG cell-attributes.
  this->AddAnnotatedAttributes(inputPDC);

  // Iterate over partitioned datasets and turn unstructured grids
  // into cell grids.
  //
  // NB: We cannot use vtkPartitionedDataSetCollection::NewIterator()
  // to fetch an iterator because there is no mapping between that
  // iterator's flat index and the flat index of the parent node ID
  // in the vtkDataAssembly.
  // For instance, given a data assembly like this:
  //   + root 1
  //     + node 2 : dataset ids 0, 3
  //     + node 3 : dataset ids 1
  //       + node 4 : dataset ids 2
  //  When a composite iterator points to a dataset held inside
  //  dataset id 1 (itself a partitioned-dataset), there is no
  //  way to discover the flat index of dataset id 1 from the
  //  (non-partitioned) child dataset or the iterator.
  //
  // Instead, we manually iterate over the collection's top-level
  // vector of partitioned-datasets in order to track the dataset ids.
  output->CopyStructure(inputPDC);
  auto numPartitionedDataSets = inputPDC->GetNumberOfPartitionedDataSets();
  for (unsigned int ii = 0; ii < numPartitionedDataSets; ++ii)
  {
    auto* pd = inputPDC->GetPartitionedDataSet(ii);
    this->Request->FlatIndex = ii;
    auto numPartitions = pd->GetNumberOfPartitions();
    for (unsigned int jj = 0; jj < numPartitions; ++jj)
    {
      if (auto* ugrid = vtkUnstructuredGrid::SafeDownCast(pd->GetPartition(jj)))
      {
        vtkNew<vtkCellGrid> cellGrid;
        if (!this->ProcessUnstructuredGrid(ugrid, cellGrid))
        {
          return 0;
        }
        output->GetPartitionedDataSet(ii)->SetPartition(jj, cellGrid);
      }
    }
  }
  this->Request->FlatIndex = static_cast<unsigned int>(-1); // Invalidate until the next run.

  return 1;
}

void vtkUnstructuredGridToCellGrid::AddAnnotatedAttributes(vtkPartitionedDataSetCollection* input)
{
  // See if we have annotations.
  auto* fieldData = input ? input->GetFieldData() : nullptr;
  if (!fieldData)
  {
    return;
  }
  static constexpr char iossAnnotations[] = "Information Records";
  auto* infoRecords = vtkStringArray::SafeDownCast(fieldData->GetAbstractArray(iossAnnotations));
  if (!infoRecords)
  {
    return;
  }
  // If we don't have a way to look up references to element blocks, we can do nothing:
  auto* dasm = input->GetDataAssembly();
  if (!dasm)
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
    auto nodeIds = dasm->SelectNodes(queries);
    if (nodeIds.empty())
    {
      vtkWarningMacro("Unmatched block \"" << escape(data[1]) << "\" in \"" << record << "\".");
      continue;
    }
    for (const auto& nodeId : nodeIds)
    {
      auto dataIds = dasm->GetDataSetIndices(nodeId, true);
      // NB: It is not an error for dataIds to be empty. (In that case, we presume the data
      //     is distributed and other ranks may have non-empty partitions of the node.)
      for (const auto& dataId : dataIds)
      {
        auto& blockData = this->Request->Annotations[dataId];
        vtkStringToken recordType(data[3]);
        vtkStringToken recordFunctionSpace(data[0]);
        vtkStringToken recordDOFSharing(data[2]);
        TranscribeQuery::BlockAttributesKey key{ recordDOFSharing, recordFunctionSpace };
        auto& blockRecord(blockData[key]);
        blockRecord.NodeIds.insert(nodeId);
        // Insert this record into the metadata for the corresponding dataset.
        // XXX(c++14)
#if __cplusplus < 201400L
        auto tokenId = recordType.GetId();
        if (tokenId == "field"_hash)
        {
          vtkStringToken fieldName(data[4]);
          blockRecord.FieldNames.insert(fieldName);
        }
        else if (tokenId == "basis"_hash)
        {
          auto descriptor = ::split(data[4], "_");
          if (descriptor.size() == 5)
          {
            if (recordFunctionSpace != descriptor[1])
            {
              vtkWarningMacro("Function space of record ("
                << recordFunctionSpace.Data() << ") and basis spec (" << descriptor[1] << ") in \""
                << record << "\" do not match. Skipping.");
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
#else
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
#endif
      }
    }
  }

  // Now, for each block, attempt to glom fields together into
  // multi-component arrays.
  for (const auto& annotation : this->Request->Annotations)
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
  for (const auto& blockEntry : this->Request->Annotations)
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
                  << dasm->GetNodePath(blockEntry.first) << "\n";
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

bool vtkUnstructuredGridToCellGrid::ProcessUnstructuredGrid(
  vtkUnstructuredGrid* input, vtkCellGrid* output)
{
  // Add every type of cell to the output (so the query
  // asks each one which input cells it can claim).
  output->Initialize();
  output->AddAllCellMetadata();

  // Now claim cells:
  this->Request->Input = input;
  this->Request->Output = output;
  this->Request->Phase = VTK_TRANSCRIBE_CELLGRID_PHASE_CLAIM;
  if (!output->Query(this->Request))
  {
    vtkErrorMacro("Cell-grid failed to claim input cells.");
    return false;
  }
  this->Request->Input = input;
  this->Request->Output = output;
  if (!this->Request->SumOutputCounts())
  {
    // TODO: Warn or error or ignore when unhandled input cells exist? Should be configurable.
    vtkWarningMacro("One or more unhandled input cells exist.");
  }
  this->Request->Phase = VTK_TRANSCRIBE_CELLGRID_PHASE_CONVERT;
  if (!output->Query(this->Request))
  {
    vtkErrorMacro("Cell-grid failed to transcribe some claimed input cells.");
    return false;
  }
  output->RemoveUnusedCellMetadata();

  // TODO: Will we ever copy schema/content information from the unstructured grid?
  output->SetSchema("dg leaf", 1);
  output->SetContentVersion(1);
  return true;
}

VTK_ABI_NAMESPACE_END
