//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/FidesDataSetWriter.h>
#include <fides/FidesWriter.h>

#if FIDES_USE_VTK
#include <fides/vtk/FidesDataSetWriterVTK.h>
#include <vtkCompositeDataSet.h>
#include <vtkDataAssembly.h>
#include <vtkInformation.h>
#include <vtkPartitionedDataSet.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkSmartPointer.h>
#endif

#if FIDES_USE_VISKORES
#include <fides/viskores/FidesDataSetWriterViskores.h>
#include <viskores/cont/PartitionedDataSet.h>
#endif

#include <map>
#include <set>
#include <stdexcept>

namespace fides
{
namespace io
{

#if FIDES_USE_VTK
namespace
{
/// Recursively translate a vtkDataAssembly node into a fides::AssemblyNode,
/// resolving dataset slot indices to item names via \c slotToName. The
/// reader-injected auto-names subtree is skipped by the caller so it does not
/// accumulate on re-write.
void ConvertAssemblyNode(vtkDataAssembly* a,
                         int node,
                         const std::vector<std::string>& slotToName,
                         fides::AssemblyNode& out)
{
  const char* nm = a->GetNodeName(node);
  out.Name = nm ? nm : "";
  for (unsigned int idx : a->GetDataSetIndices(node, /*traverse_subtree=*/false))
  {
    if (idx < slotToName.size())
    {
      out.Datasets.push_back(slotToName[idx]);
    }
  }
  for (int child : a->GetChildNodes(node, /*traverse_subtree=*/false))
  {
    out.Children.emplace_back();
    ConvertAssemblyNode(a, child, slotToName, out.Children.back());
  }
}
}
#endif

FidesDataSetWriter::FidesDataSetWriter(const std::string& outputFile, const std::string& outputMode)
  : Writer(new FidesWriter(outputFile, outputMode))
{
}

#if FIDES_USE_MPI
FidesDataSetWriter::FidesDataSetWriter(const std::string& outputFile,
                                       MPI_Comm comm,
                                       const std::string& outputMode)
  : Writer(new FidesWriter(outputFile, comm, outputMode))
{
}
#endif

FidesDataSetWriter::~FidesDataSetWriter() = default;

void FidesDataSetWriter::SetAdiosConfigFile(const std::string& adiosConfigPath)
{
  this->Writer->SetAdiosConfigFile(adiosConfigPath);
}

void FidesDataSetWriter::SetEngineParameters(
  const std::unordered_map<std::string, std::string>& params)
{
  // FidesWriter::SetEngineParameters takes std::map; convert.
  std::map<std::string, std::string> m(params.begin(), params.end());
  this->Writer->SetEngineParameters(m);
}

void FidesDataSetWriter::SetWriteFields(const std::vector<std::string>& fieldNames)
{
  std::set<std::string> s(fieldNames.begin(), fieldNames.end());
  this->FieldsToWrite = s;
  this->WriteAll = false;
  this->Writer->SetWriteFields(s);
}

void FidesDataSetWriter::SetWriteAllFields(bool writeAll)
{
  this->WriteAll = writeAll;
  if (writeAll)
  {
    this->FieldsToWrite.clear();
  }
  this->Writer->SetWriteAllFields(writeAll);
}

void FidesDataSetWriter::BeginStep()
{
  this->Writer->BeginStep();
}

void FidesDataSetWriter::SetCurrentTime(double t)
{
  this->Writer->SetCurrentTime(t);
}

void FidesDataSetWriter::Write(const fides::VTKCollection& dataSets)
{
#if FIDES_USE_VTK
  auto extraction = fides::ExtractVTKPartitions(
    dataSets.Get(), this->WriteAll ? std::set<std::string>() : this->FieldsToWrite);
  // ExtractVTKPartitions throws if both kinds are present in one step,
  // so exactly one of these vectors is non-empty here (or both are
  // empty, which also means "nothing to do").
  if (!extraction.CellGrids.empty())
  {
    this->Writer->Write(extraction.CellGrids);
  }
  else
  {
    this->Writer->Write(extraction.DataSets);
  }
#else
  (void)dataSets;
  throw std::runtime_error("Fides was not built with VTK support");
#endif
}

void FidesDataSetWriter::Write(const fides::ViskoresCollection& dataSets)
{
#if FIDES_USE_VISKORES
  auto partitions = fides::ExtractViskoresPartitions(
    dataSets, this->WriteAll ? std::set<std::string>() : this->FieldsToWrite);
  this->Writer->Write(partitions);
#else
  (void)dataSets;
  throw std::runtime_error("Fides was not built with Viskores support");
#endif
}

void FidesDataSetWriter::Write(fides::DataContainer& container)
{
#if FIDES_USE_VTK
  if (auto* vtk = fides::GetDataAs<vtkSmartPointer<vtkPartitionedDataSet>>(container))
  {
    this->Write(*vtk);
    return;
  }
#endif
#if FIDES_USE_VISKORES
  if (auto* vec = fides::GetDataAs<std::vector<viskores::cont::DataSet>>(container))
  {
    this->Write(viskores::cont::PartitionedDataSet(*vec));
    return;
  }
#endif
  throw std::runtime_error("DataContainer does not hold a supported VTK or Viskores collection");
}

void FidesDataSetWriter::WriteCollection(const fides::VTKPDC& pdc)
{
#if FIDES_USE_VTK
  auto* p = pdc.Get();
  if (!p)
  {
    throw std::runtime_error("WriteCollection: null vtkPartitionedDataSetCollection");
  }
  const std::set<std::string> fields =
    this->WriteAll ? std::set<std::string>() : this->FieldsToWrite;

  std::vector<fides::CollectionItem> items;
  std::vector<std::string> slotToName;
  for (unsigned int i = 0; i < p->GetNumberOfPartitionedDataSets(); ++i)
  {
    fides::CollectionItem item;
    item.Name = "dataset_" + std::to_string(i);
    if (p->HasMetaData(i) && p->GetMetaData(i)->Has(vtkCompositeDataSet::NAME()))
    {
      item.Name = p->GetMetaData(i)->Get(vtkCompositeDataSet::NAME());
    }
    auto extraction = fides::ExtractVTKPartitions(p->GetPartitionedDataSet(i), fields);
    // WriteCollection currently only carries DataSet items; cellgrid PDC
    // items are a separate (not-yet-implemented) feature. Reject early
    // with a clear error rather than silently dropping the cellgrid.
    if (!extraction.CellGrids.empty())
    {
      throw std::runtime_error("WriteCollection: dataset '" + item.Name +
                               "' contains vtkCellGrid partitions; multi-item PDCs containing "
                               "cellgrids are not yet supported.");
    }
    item.Partitions = std::move(extraction.DataSets);
    slotToName.push_back(item.Name);
    items.push_back(std::move(item));
  }

  // Translate the schema-declared assembly subtrees (everything under
  // the root except the reader-synthesized auto-names subtree, which is
  // regenerated on read).
  fides::AssemblyNode assembly;
  bool hasAssembly = false;
  if (auto* a = p->GetDataAssembly())
  {
    const char* rootName = a->GetRootNodeName();
    assembly.Name = rootName ? rootName : "";
    const int root = vtkDataAssembly::GetRootNode();
    for (int child : a->GetChildNodes(root, /*traverse_subtree=*/false))
    {
      const char* cn = a->GetNodeName(child);
      if (cn && std::string(cn) == fides::kAutoNamesAssemblySubtree)
      {
        continue;
      }
      assembly.Children.emplace_back();
      ConvertAssemblyNode(a, child, slotToName, assembly.Children.back());
      hasAssembly = true;
    }
  }
  this->Writer->WriteCollection(items, hasAssembly ? &assembly : nullptr);
#else
  (void)pdc;
  throw std::runtime_error("Fides was not built with VTK support");
#endif
}

void FidesDataSetWriter::WriteCollection(const std::vector<std::string>& names,
                                         const std::vector<fides::ViskoresCollection>& datasets)
{
#if FIDES_USE_VISKORES
  if (names.size() != datasets.size())
  {
    throw std::runtime_error("WriteCollection: names and datasets size mismatch (" +
                             std::to_string(names.size()) + " vs " +
                             std::to_string(datasets.size()) + ").");
  }
  const std::set<std::string> fields =
    this->WriteAll ? std::set<std::string>() : this->FieldsToWrite;
  std::vector<fides::CollectionItem> items;
  items.reserve(datasets.size());
  for (size_t i = 0; i < datasets.size(); ++i)
  {
    items.push_back({ names[i], fides::ExtractViskoresPartitions(datasets[i], fields) });
  }
  // Viskores has no assembly concept.
  this->Writer->WriteCollection(items, nullptr);
#else
  (void)names;
  (void)datasets;
  throw std::runtime_error("Fides was not built with Viskores support");
#endif
}

void FidesDataSetWriter::EndStep()
{
  this->Writer->EndStep();
}

void FidesDataSetWriter::Close()
{
  this->Writer->Close();
}

bool FidesDataSetWriter::IsStepOpen() const
{
  return this->Writer->IsStepOpen();
}

} // namespace io
} // namespace fides
