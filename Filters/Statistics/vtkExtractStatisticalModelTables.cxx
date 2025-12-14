// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2025 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkExtractStatisticalModelTables.h"

#include "vtkDataAssembly.h"
#include "vtkDataAssemblyVisitor.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkStatisticalModel.h"
#include "vtkStringFormatter.h"
#include "vtkTable.h"

VTK_ABI_NAMESPACE_BEGIN

namespace
{

void AddModelToAssembly(vtkPartitionedDataSetCollection* out, vtkStatisticalModel* model,
  vtkDataAssembly* assy, int rootAssyNode)
{
  if (!model || model->IsEmpty())
  {
    return;
  }

  // Iterate model tables, adding each to a new collection of \a out.
  // Add hierarchy to \a rootAssyNode of \a assy.
  unsigned int nextPartition = out->GetNumberOfPartitionedDataSets();
  for (int tt = 0; tt < vtkStatisticalModel::GetNumberOfTableTypes(); ++tt)
  {
    auto tableType = static_cast<vtkStatisticalModel::TableType>(tt);
    int numTablesThisType = model->GetNumberOfTables(tt);
    if (!numTablesThisType)
    {
      continue; // no tables to add
    }
    int typeNode = assy->AddNode(vtkStatisticalModel::GetTableTypeName(tableType), rootAssyNode);
    for (int ii = 0; ii < numTablesThisType; ++ii)
    {
      unsigned int dsidx = nextPartition++;
      out->SetPartition(dsidx, 0, model->GetTable(tt, ii));
      int node = assy->AddNode(model->GetTableName(tt, ii).c_str(), typeNode);
      assy->AddDataSetIndex(node, dsidx);
    }
  }
}

class ModelExtractor : public vtkDataAssemblyVisitor
{
public:
  static ModelExtractor* New();
  vtkTypeMacro(ModelExtractor, vtkDataAssemblyVisitor);

  vtkPartitionedDataSetCollection* In;
  vtkPartitionedDataSetCollection* Out;
  vtkDataAssembly* AssemblyOut;

  void Initialize(vtkPartitionedDataSetCollection* pdc, vtkPartitionedDataSetCollection* out,
    vtkDataAssembly* resultAssy)
  {
    this->In = pdc;
    this->Out = out;
    this->AssemblyOut = resultAssy;
  }

  void Visit(int nodeId) override
  {
    auto indices = this->GetCurrentDataSetIndices();
    int parentNode = nodeId;
    int midIdx = indices.size() > 1 ? 0 : -1;
    for (const auto& index : indices)
    {
      auto* pd = In->GetPartitionedDataSet(index);
      if (!pd || pd->GetNumberOfPartitions() == 0)
      {
        continue;
      }
      unsigned int np = pd->GetNumberOfPartitions();
      if (np > 1 && midIdx < 0)
      {
        midIdx = 0;
      }
      // Loop over partitions. If any is a statistical model, add its tables.
      for (unsigned int ii = 0; ii < np; ++ii)
      {
        if (auto* model = vtkStatisticalModel::SafeDownCast(pd->GetPartitionAsDataObject(ii)))
        {
          if (midIdx >= 0)
          {
            std::string midNodeName = vtk::to_string(midIdx++);
            parentNode = this->AssemblyOut->AddNode(midNodeName.c_str(), nodeId);
          }
          AddModelToAssembly(this->Out, model, this->AssemblyOut, parentNode);
        }
      }
    }
  }
};

} // anonymous namespace

vtkStandardNewMacro(vtkExtractStatisticalModelTables);
vtkStandardNewMacro(ModelExtractor);

vtkExtractStatisticalModelTables::vtkExtractStatisticalModelTables() = default;
vtkExtractStatisticalModelTables::~vtkExtractStatisticalModelTables() = default;

void vtkExtractStatisticalModelTables::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

int vtkExtractStatisticalModelTables::FillInputPortInformation(int port, vtkInformation* info)
{
  (void)port;
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkStatisticalModel");
  return 1;
}

int vtkExtractStatisticalModelTables::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inInfoVec, vtkInformationVector* outInfoVec)
{
  auto* model = vtkStatisticalModel::GetData(inInfoVec[0]);
  auto* pdc = vtkPartitionedDataSetCollection::GetData(inInfoVec[0]);
  auto* out = vtkPartitionedDataSetCollection::GetData(outInfoVec);

  vtkNew<vtkDataAssembly> resultAssy;

  if (model)
  {
    AddModelToAssembly(out, model, resultAssy, /*root node of model*/ 0);
  }
  else if (pdc)
  {
    // Copy the input's structure so all node IDs match, but remove dataset references:
    auto* assemblyIn = pdc->GetDataAssembly();
    resultAssy->DeepCopy(assemblyIn);
    resultAssy->RemoveAllDataSetIndices(0, /*traverse_subtree*/ true);
    // Create a visitor for the input collection.
    vtkNew<ModelExtractor> extractor;
    extractor->Initialize(pdc, out, resultAssy);
    // Add new node IDs for statistical models by traversing the input:
    assemblyIn->Visit(extractor);
  }
  else
  {
    auto* data = vtkDataObject::GetData(inInfoVec[0]);
    vtkErrorMacro("Unhandled input type \"" << (data ? data->GetClassName() : "null") << "\".");
    return 0;
  }
  out->SetDataAssembly(resultAssy);

  return 1;
}

VTK_ABI_NAMESPACE_END
