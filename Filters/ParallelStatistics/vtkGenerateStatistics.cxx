// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright 2025 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkGenerateStatistics.h"

#include "vtkAlgorithm.h"
#include "vtkCellData.h"
#include "vtkCellGrid.h"
#include "vtkCellGridSampleQuery.h"
#include "vtkCellSizeFilter.h"
#include "vtkCompiler.h"
#include "vtkCompositeDataSet.h"
#include "vtkConstantArray.h"
#include "vtkDataArray.h"
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyVisitor.h"
#include "vtkDataObject.h"
#include "vtkDataObjectCollection.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkDoubleArray.h"
#include "vtkFiltersCellGrid.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkGenericDataObjectWriter.h"
#include "vtkGraph.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkReservoirSampler.h"
#include "vtkSMPTools.h"
#include "vtkStatisticalModel.h"
#include "vtkStatisticsAlgorithm.h"
#include "vtkStatisticsAlgorithmPrivate.h"
#include "vtkStringArray.h"
#include "vtkStringFormatter.h"
#include "vtkTable.h"
#include "vtkUniformGridAMR.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVariantArray.h"

#include <algorithm>
#include <set>
#include <sstream>

// Set VTK_DBG_INPUTDATA to a non-zero value to write input tables to disk
// for each rank (after subsampling, ghost-skipping, weighting, and
// re-centering to a common field association).
#define VTK_DBG_INPUTDATA 0
// Set VTK_DBG_ASSEMBLY to a non-zero value to debug aggregation of models
// into data-assemblies.
#define VTK_DBG_ASSEMBLY 0
// Set VTK_DBG_MODELDATA to a non-zero value to log activity communicating
// model data between ranks to files in /tmp:
#define VTK_DBG_MODELDATA 0
#if VTK_DBG_MODELDATA
#include <fstream>
#endif

VTK_ABI_NAMESPACE_BEGIN

#ifdef VTK_COMPILER_MSVC
#pragma float_control(precise, on) // enable precise semantics
#pragma fp_contract(off)           // disable contractions
#endif

namespace
{
#if VTK_DBG_INPUTDATA
char runId = 'A';
#endif // VTK_DBG_INPUTDATA

#if VTK_DBG_ASSEMBLY
int modelCardinality(vtkStatisticalModel* model)
{
  if (!model || model->GetNumberOfTables(0) <= 0)
  {
    return 0;
  }
  vtkTable* tab = model->GetTable(0, 0);
  return tab->GetValueByName(0, "Cardinality").ToInt();
}
#endif // VTK_DBG_ASSEMBLY

#if VTK_DBG_MODELDATA
void reportNode(
  vtkPartitionedDataSetCollection* pdc, vtkDataAssembly* da, int nodeId, vtkIndent& indent)
{
  auto dnodes = da->GetDataSetIndices(nodeId, /*traverse_subtree*/ false);
  if (!dnodes.empty())
  {
    // Print out datasets attached to nodeId:
    std::cout << indent << da->GetNodePath(nodeId) << ":";
    for (const auto& dnode : dnodes)
    {

      std::cout << " " << dnode << "(";
      auto* pds = pdc->GetPartitionedDataSet(dnode);
      if (!pds)
      {
        std::cout << "--)\n";
        continue;
      }
      int nn = pds->GetNumberOfPartitions();
      for (int ii = 0; ii < nn; ++ii)
      {
        auto* obj = pds->GetPartitionAsDataObject(ii);
        if (ii > 0)
        {
          std::cout << " ";
        }
        std::cout << obj << " " << (obj ? obj->GetClassName() : "(null)");
        if (auto* model = vtkStatisticalModel::SafeDownCast(obj))
        {
          if (model->IsEmpty())
          {
            std::cout << "(empty)";
          }
          else
          {
            std::cout << "("
                      << model->GetTable(vtkStatisticalModel::Learned, 0)
                           ->GetValueByName(0, "Cardinality")
                           .ToInt()
                      << " samples)";
          }
        }
        std::cout << ")";
      }
    }
    std::cout << "\n";
  }

  // Now recurse over children, if any:
  auto next = indent.GetNextIndent();
  for (int cc = 0; cc < da->GetNumberOfChildren(nodeId); ++cc)
  {
    int child = da->GetChild(nodeId, cc);
    reportNode(pdc, da, child, next);
  }
}

void reportModelTree(vtkPartitionedDataSetCollection* pdc)
{
  vtkIndent indent(4);
  auto* da = pdc->GetDataAssembly();
  int nodeId = 0;
  reportNode(pdc, da, 0, indent);
}

void reportRanks(const std::string& msg, vtkPartitionedDataSetCollection* pdc,
  vtkMultiProcessController* controller)
{
  int rank = controller ? controller->GetLocalProcessId() : 0;
  int numRanks = controller ? controller->GetNumberOfProcesses() : 1;
  for (int rr = 0; rr < numRanks; ++rr)
  {
    if (controller)
    {
      controller->Barrier();
    }
    if (rank == 0 && rr == 0)
    {
      std::cout << msg << "\n";
    }
    if (rr == rank)
    {
      std::cout << "  Rank " << (rank + 1) << " / " << numRanks << ":\n";
      reportModelTree(pdc);
    }
  }
}
#endif // VTK_DBG_MODELDATA

#if VTK_DBG_INPUTDATA
void DumpTo(ostream& os, vtkTable* table, unsigned int colWidth, int rowLimit)
{
  if (!table || !table->GetNumberOfColumns())
  {
    os << "++\n++\n";
    return;
  }

  std::string lineStr;
  for (int c = 0; c < table->GetNumberOfColumns(); ++c)
  {
    lineStr += "+-";

    for (unsigned int i = 0; i < colWidth; ++i)
    {
      lineStr += "-";
    }
  }
  lineStr += "-+\n";

  os << lineStr;

  for (int c = 0; c < table->GetNumberOfColumns(); ++c)
  {
    os << "| ";
    const char* name = table->GetColumnName(c);
    std::string str = name ? name : "";

    if (colWidth < str.length())
    {
      os << str.substr(0, colWidth);
    }
    else
    {
      os << str;
      for (unsigned int i = static_cast<unsigned int>(str.length()); i < colWidth; ++i)
      {
        os << " ";
      }
    }
  }

  os << " |\n" << lineStr;

  if (rowLimit != 0)
  {
    for (vtkIdType r = 0; r < table->GetNumberOfRows(); ++r)
    {
      for (int c = 0; c < table->GetNumberOfColumns(); ++c)
      {
        os << "| ";
        std::string str = table->GetValue(r, c).ToString();

        if (colWidth < str.length())
        {
          os << str.substr(0, colWidth);
        }
        else
        {
          os << str;
          for (unsigned int i = static_cast<unsigned int>(str.length()); i < colWidth; ++i)
          {
            os << " ";
          }
        }
      }
      os << " |\n";
      if (rowLimit != -1 && r >= rowLimit)
        break;
    }
    os << lineStr;
    os.flush();
  }
}
#endif // VTK_DBG_INPUTDATA

} // anonymous namespace

// This visitor is used to traverse input data, creating a model of
// each node with non-empty dataset indices in it. The model will
// either be copied into the output model tree or merged with other
// model data (depending on whether this->Self->GetSingleModel()
// is set).
class vtkGenerateStatistics::StatisticsAccumulator : public vtkDataAssemblyVisitor
{
public:
  static StatisticsAccumulator* New();
  vtkTypeMacro(StatisticsAccumulator, vtkDataAssemblyVisitor);

  vtkGenerateStatistics* Self{ nullptr };
  vtkPartitionedDataSetCollection* Data{ nullptr };
  vtkPartitionedDataSetCollection* ModelTree{ nullptr };
  vtkNew<vtkStatisticalModel> LocalModel;
  vtkSmartPointer<vtkStatisticalModel> CurrentModel;

  /// If \a nodeId has any dataset indices, compute a model
  /// for the sum of all of them.
  void Visit(int nodeId) override
  {
    if (!this->Self || !this->ModelTree)
    {
      return;
    }

    if (!this->CurrentModel)
    {
      this->CurrentModel = vtkSmartPointer<vtkStatisticalModel>::New();
    }
    // Find all the vtkPartitionedDataSet instances attached to \a nodeId:
    auto partitionIndices =
      this->GetAssembly()->GetDataSetIndices(nodeId, /* traverse_subtree */ false);
    vtkNew<vtkDataObjectCollection> models;
    for (const auto& partitionIndex : partitionIndices)
    {
      auto* pds = this->Data->GetPartitionedDataSet(partitionIndex);
      if (!pds)
      {
        continue;
      }
      for (unsigned int ii = 0; ii < pds->GetNumberOfPartitions(); ++ii)
      {
        this->LocalModel->Initialize();
#if VTK_DBG_INPUTDATA
        std::cerr << "Stats for partition " << ii << " of dataset " << partitionIndex << " (node "
                  << nodeId << ") will be modeData_" << runId << ".\n";
#endif
        if (this->Self->RequestDataNonComposite(
              pds->GetPartitionAsDataObject(ii), this->LocalModel) == 1)
        {
          if (this->LocalModel->IsEmpty())
          {
            continue;
          }
          else if (this->CurrentModel->IsEmpty())
          {
            this->CurrentModel->DeepCopy(this->LocalModel);
          }
          else
          {
            // Aggregate this dataset's model into the current model:
            vtkNew<vtkStatisticalModel> temp;
            temp->DeepCopy(this->CurrentModel);
            models->RemoveAllItems();
            models->AddItem(this->LocalModel);
            models->AddItem(temp);
            this->Self->GetStatisticsAlgorithm()->Aggregate(models, this->CurrentModel);
          }
        }
      }
    }
    // Now, our CurrentModel includes all the samples from \a nodeId.
    // If we aren't aggregating across the entire vtkDataAssembly, it needs
    // to be placed into the model hierarchy (which matches the data hierarchy)
    // and a new CurrentModel created for the next vtkDataAssembly node.
    if (!this->Self->GetSingleModel() && !this->CurrentModel->IsEmpty())
    {
      unsigned int modelDataSetIndex = this->ModelTree->GetNumberOfPartitionedDataSets();
      this->ModelTree->SetPartition(modelDataSetIndex, 0, this->CurrentModel);
      this->ModelTree->GetDataAssembly()->AddDataSetIndex(nodeId, modelDataSetIndex);
      this->CurrentModel = nullptr;
    }
  }
};

// Merge statistics models in matching nodes of two trees.
// The trees are assumed to be identical (i.e., having the
// same node IDs) because there is not an easy way to compare
// two distinct trees to find matching nodes.
//
// This visitor is used inside vtkGenerateStatistics::MergeModelTrees()
// to merge a tree of model data accumulated on a remote rank
// (and transmitted to this rank for processing) with the models
// in this rank's model-hierarchy.
//
// At completion, the \a TargetData and \a TargetAssembly contain
// statistical model objects representing all the samples in
// both the \a SourceData and \a TargetData trees.
class vtkModelMerger : public vtkDataAssemblyVisitor
{
public:
  static vtkModelMerger* New();
  vtkTypeMacro(vtkModelMerger, vtkDataAssemblyVisitor);

  vtkPartitionedDataSetCollection* SourceData{ nullptr };
  vtkPartitionedDataSetCollection* TargetData{ nullptr };
  vtkDataAssembly* SourceAssembly{ nullptr };
  vtkDataAssembly* TargetAssembly{ nullptr };
  bool Error{ false }; // Set to true when an error occurs.
  vtkStatisticsAlgorithm* Algorithm{ nullptr };
  vtkNew<vtkDataObjectCollection> Collection;
  vtkGenerateStatistics* Self{ nullptr };

  void Visit(int nodeId) override
  {
    std::string srcName = this->SourceAssembly->GetNodeName(nodeId);
    std::string tgtName = this->TargetAssembly->GetNodeName(nodeId);
    if (srcName != tgtName)
    {
      vtkErrorWithObjectMacro(this->TargetAssembly,
        "Mismatched nodes at " << nodeId << ": \"" << srcName << "\" vs \"" << tgtName << "\".");
      this->Error = true;
      return;
    }
    auto srcDataIndices =
      this->SourceAssembly->GetDataSetIndices(nodeId, /* traverse_subtree */ false);
    auto tgtDataIndices =
      this->TargetAssembly->GetDataSetIndices(nodeId, /* traverse_subtree */ false);
    vtkStatisticalModel* srcModel = nullptr;
    vtkStatisticalModel* tgtModel = nullptr;
    for (const auto& dataIndex : srcDataIndices)
    {
      auto* model =
        vtkStatisticalModel::SafeDownCast(this->SourceData->GetPartitionAsDataObject(dataIndex, 0));
      if (model)
      {
        srcModel = model;
        break;
      }
    }
    unsigned int tgtDataIndex = ~static_cast<unsigned int>(0);
    for (const auto& dataIndex : tgtDataIndices)
    {
      auto* model =
        vtkStatisticalModel::SafeDownCast(this->TargetData->GetPartitionAsDataObject(dataIndex, 0));
      if (model)
      {
        tgtDataIndex = dataIndex;
        tgtModel = model;
        break;
      }
    }
    if (tgtDataIndex == static_cast<unsigned int>(~0))
    {
      // If we have no data for the target node but do have data from the source node,
      // we need a place to copy the source model to. Append it to the end of TargetData:
      tgtDataIndex = this->TargetData->GetNumberOfPartitionedDataSets();
    }
    if (tgtModel && srcModel)
    {
      vtkNew<vtkStatisticalModel> temp;
      temp->DeepCopy(tgtModel);
#if VTK_DBG_ASSEMBLY
      auto sc = modelCardinality(srcModel);
      auto tc = modelCardinality(tgtModel);
#endif
      this->Collection->RemoveAllItems();
      this->Collection->AddItem(srcModel);
      this->Collection->AddItem(temp);
      if (!this->Algorithm->Aggregate(this->Collection, tgtModel))
      {
        vtkErrorWithObjectMacro(this->Algorithm, "Failed to merge statistical models.");
      }
#if VTK_DBG_ASSEMBLY
      // For debugging model merges, print cardinality of source and target tables:
      auto tc2 = modelCardinality(tgtModel);
      auto rank = this->Self->GetController()->GetLocalProcessId();
      std::cerr << "rank " << rank << " merge s " << sc << " t " << tc << " → " << tc2 << "\n";
#endif
    }
    else if (srcModel)
    {
      vtkNew<vtkStatisticalModel> temp;
      temp->DeepCopy(srcModel);
      // Copy the source model to the target since the target is empty.
      this->TargetData->SetPartition(tgtDataIndex, 0, temp);
      // Add the dataset index to the assembly. If already present, this will do nothing.
      this->TargetAssembly->AddDataSetIndex(nodeId, tgtDataIndex);
    }
    else
    {
      // Do nothing. There is no other model to merge with it and either the target is
      // non-null (and is retained as the destination model) or both are null.
    }
  }
};

vtkStandardNewMacro(vtkModelMerger);
vtkStandardNewMacro(vtkGenerateStatistics::StatisticsAccumulator);
vtkStandardNewMacro(vtkGenerateStatistics);
vtkCxxSetObjectMacro(vtkGenerateStatistics, Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkGenerateStatistics, StatisticsAlgorithm, vtkStatisticsAlgorithm);

vtkGenerateStatistics::vtkGenerateStatistics()
{
  static bool once = false;
  if (!once)
  {
    vtkFiltersCellGrid::RegisterCellsAndResponders();
  }

  this->P = std::make_unique<vtkStatisticsAlgorithmPrivate>();
  this->AttributeMode = vtkDataObject::POINT;
  this->WeightByCellMeasure = false;
  this->SingleModel = true;
  this->TrainingFraction = 0.1;
  this->SetNumberOfInputPorts(1);  // data to model
  this->SetNumberOfOutputPorts(1); // model of data
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->StatisticsAlgorithm = nullptr;
}

vtkGenerateStatistics::~vtkGenerateStatistics()
{
  this->SetController(nullptr);
  this->SetStatisticsAlgorithm(nullptr);
}

void vtkGenerateStatistics::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AttributeMode: " << this->AttributeMode << "\n";
  os << indent << "TrainingFraction: " << this->TrainingFraction << "\n";
  os << indent << "SingleModel: " << (this->SingleModel ? "Y" : "N") << "\n";
  os << indent << "WeightByCellMeasure: " << (this->WeightByCellMeasure ? "Y" : "N") << "\n";
  os << indent << "Controller:" << (this->Controller ? "\n" : " none\n");
  if (this->Controller)
  {
    vtkIndent i2 = indent.GetNextIndent();
    this->Controller->PrintSelf(os, i2);
  }
}

int vtkGenerateStatistics::GetNumberOfAttributeArrays()
{
  vtkDataObject* dobj = this->GetInputDataObject(0, 0); // First input is always the leader
  if (!dobj)
  {
    return 0;
  }

  if (auto* cellGrid = vtkCellGrid::SafeDownCast(dobj))
  {
    return static_cast<int>(cellGrid->GetUnorderedCellAttributeIds().size());
  }

  vtkFieldData* fdata = dobj->GetAttributesAsFieldData(this->AttributeMode);
  if (!fdata)
  {
    return 0;
  }

  return fdata->GetNumberOfArrays();
}

const char* vtkGenerateStatistics::GetAttributeArrayName(int nn)
{
  vtkDataObject* dobj = this->GetInputDataObject(0, 0); // First input is always the leader
  if (!dobj)
  {
    return nullptr;
  }

  if (auto* cellGrid = vtkCellGrid::SafeDownCast(dobj))
  {
    auto ids = cellGrid->GetUnorderedCellAttributeIds();
    if (nn < 0 || nn >= static_cast<int>(ids.size()))
    {
      return nullptr;
    }
    auto* cellAtt = cellGrid->GetCellAttributeById(ids[nn]);
    return cellAtt ? cellAtt->GetName().Data().c_str() : nullptr;
  }

  vtkFieldData* fdata = dobj->GetAttributesAsFieldData(this->AttributeMode);
  if (!fdata)
  {
    return nullptr;
  }

  int numArrays = fdata->GetNumberOfArrays();
  if (nn < 0 || nn > numArrays)
  {
    return nullptr;
  }

  vtkAbstractArray* arr = fdata->GetAbstractArray(nn);
  if (!arr)
  {
    return nullptr;
  }

  return arr->GetName();
}

int vtkGenerateStatistics::GetAttributeArrayStatus(const char* arrName)
{
  return this->P->Has(arrName) ? 1 : 0;
}

void vtkGenerateStatistics::EnableAttributeArray(const char* arrName)
{
  if (arrName)
  {
    if (this->P->SetBufferColumnStatus(arrName, 1))
    {
      this->Modified();
    }
  }
}

void vtkGenerateStatistics::ClearAttributeArrays()
{
  if (this->P->ResetBuffer())
  {
    this->Modified();
  }
}

int vtkGenerateStatistics::FillInputPortInformation(int port, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  if (port == 0)
  {
    return 1;
  }
  return 0;
}

int vtkGenerateStatistics::RequestData(
  vtkInformation* vtkNotUsed(request), vtkInformationVector** input, vtkInformationVector* output)
{
  vtkDataObject* dataObjIn = vtkDataObject::GetData(input[0], 0);
  if (!dataObjIn)
  {
    // Silently ignore missing data.
    return 1;
  }

  int numArrays = this->GetNumberOfInputArraySpecifications();
  if (numArrays <= 0)
  {
    // If we have variables specified by the EnableAttributeArray API,
    // use it to populate "SetInputArraysToProcess".
    if (!this->P->Buffer.empty())
    {
      this->P->AddBufferToRequests();
    }
    int aa = 0;
    for (const auto& request : this->P->Requests)
    {
      for (const auto& name : request)
      {
        this->SetInputArrayToProcess(
          aa, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, name.c_str());
        ++aa;
      }
    }
    // Silently ignore empty requests.
    numArrays = this->GetNumberOfInputArraySpecifications();
    if (numArrays <= 0)
    {
      return 1;
    }
  }

  // Get output model data and sci-viz data.
  auto* modelObjOu = vtkPartitionedDataSetCollection::GetData(output, 0);
  if (!modelObjOu)
  {
    // Silently ignore missing data.
    return 1;
  }

  // TODO: Perform a "pre-run" stage to compute total number of samples across
  //       all blocks in all partitions on all ranks? If so, we should then
  //       have a target sample size on a per-leaf basis. This stage is only
  //       needed if users are allowed to specify a fixed sample size rather
  //       than a training *fraction*.

  int stat = this->RequestLocalDataDispatch(dataObjIn, modelObjOu);

  // The RequestLocalDataDispatch() above requires no communication and does
  // all the local model aggregation possible. The remaining global model(s)
  // then need to be collectively aggregated (in pairs of ranks with ⌈log₂(N)⌉
  // merges for N ranks) resulting in the final model aggregated on rank 0.
  // The final model(s) should then be broadcast from rank 0 to all ranks so
  // model assessment can occur in parallel for downstream filters.
  stat |= this->MergeRemoteModels(modelObjOu);

  // Finally, we run the statistics algorithm on each rank to compute derived
  // values.
  stat |= this->ComputeDerivedData(modelObjOu);

  return stat;
}

int vtkGenerateStatistics::RequestLocalDataDispatch(
  vtkDataObject* dataObject, vtkPartitionedDataSetCollection* modelTree)
{
  // Either we have a composite input dataset or a single data object of interest.
  vtkCompositeDataSet* compDataObjIn = vtkCompositeDataSet::SafeDownCast(dataObject);
  if (compDataObjIn)
  {
    // We handle two cases for now: vtkStatisticalModel and vtkPartitionedData.
    //
    // For the case of vtkStatisticalModel, we require either a
    // vtkDataAssembly or (for vtkUniformGridAMR) vtkAMRMetaData.
    // In either case, we construct either a single model from all matching leaf data
    // or a model per entry of the vtkDataAssembly/vtkAMRMetaData tree; the difference is
    // that when SingleModel is false, we key models for AMR data based on their (level, block)
    // index while for other PDCs, we key models to match vtkDataAssembly nodes.
    if (auto* amr = vtkUniformGridAMR::SafeDownCast(compDataObjIn))
    {
      return this->RequestDataAMR(amr, modelTree);
    }
    else if (auto* pd = vtkPartitionedDataSet::SafeDownCast(compDataObjIn))
    {
      // Unlike other composite data, a partitioned dataset will always result in a
      // single model.
      vtkNew<vtkStatisticalModel> model;
      if (this->RequestDataPD(pd, model) == 1)
      {
        modelTree->SetPartition(0, 0, model);
        vtkNew<vtkDataAssembly> tree;
        int node = tree->AddNode("Statistics", 0);
        tree->AddDataSetIndex(node, 0);
        modelTree->SetDataAssembly(tree);
        return 1;
      }
      return 0;
    }
    // If given a multi-block dataset, this will error out:
    return this->RequestDataPDC(
      vtkPartitionedDataSetCollection::SafeDownCast(compDataObjIn), modelTree);
  }

  // The remaining data types we handle always result in a single model rather
  // than possibly a hierarchy of models.
  vtkNew<vtkStatisticalModel> model;
  vtkNew<vtkDataAssembly> tree;
  modelTree->SetPartition(0, 0, model);
  int node = tree->AddNode("Statistics", 0);
  tree->AddDataSetIndex(node, 0);
  modelTree->SetDataAssembly(tree);

  return this->RequestDataNonComposite(dataObject, model);
}

int vtkGenerateStatistics::RequestDataNonComposite(
  vtkDataObject* dataObject, vtkStatisticalModel* model)
{
  // We handle several cases: vtkDataSet, vtkCellGrid, vtkGraph, and vtkTable.
  // However, these are all handled with two code paths: one for vtkCellGrid and
  // one for the remaining (as each can just fetch vtkDataSetAttributes via
  // vtkDataObject::GetAttributes()).
  if (auto* cg = vtkCellGrid::SafeDownCast(dataObject))
  {
    return this->RequestDataCellGrid(cg, model);
  }
  return this->RequestDataPlain(dataObject, model);
}

int vtkGenerateStatistics::RequestDataAMR(
  vtkUniformGridAMR* amr, vtkPartitionedDataSetCollection* modelTree)
{
  // TODO: Eventually, we should handle the case for \a SingleModel set to false.
  //       But since there is not an obvious use case at the moment and since it
  //       greatly simplifies things, we just force a single model for AMR data.
  bool prevSingleModel = this->SingleModel;
  this->SingleModel = true;
  this->RequestDataPDC(amr, modelTree);
  this->SingleModel = prevSingleModel;
  return 1;
}

int vtkGenerateStatistics::RequestDataPDC(
  vtkPartitionedDataSetCollection* pdc, vtkPartitionedDataSetCollection* modelTree)
{
  // Note: because a PDC may never contain another PDC, we know the
  // accumulator – which calls RequestLocalDataDispatch() on its assembly
  // nodes – will never recurse.
  vtkNew<StatisticsAccumulator> accumulator;
  accumulator->ModelTree = modelTree;
  accumulator->Data = pdc;
  accumulator->Self = this;
  bool didVisit = false;
  if (!this->SingleModel)
  {
    if (auto* assy = pdc->GetDataAssembly())
    {
      // Copy the assembly from the source dataset into the output model
      // so we can add models in locations that match the data sources.
      // Note that by copying the assembly before any models are added,
      // we preserve not just the structure of the assembly but also the
      // node numberings.
      auto* modelAssembly = modelTree->GetDataAssembly();
      if (!modelAssembly)
      {
        vtkNew<vtkDataAssembly> newModelAssembly;
        modelTree->SetDataAssembly(newModelAssembly);
        modelAssembly = modelTree->GetDataAssembly();
      }
      modelAssembly->DeepCopy(assy);
      modelAssembly->RemoveAllDataSetIndices(/*node*/ 0, /*recurse*/ true);
#if VTK_DBG_ASSEMBLY
      vtkIndent indent(2);
      std::cout << "About to traverse PDC and create multiple models.\n"
                   "The model hierarchy is\n"
                << modelAssembly->SerializeToXML(indent) << "\n";
#endif
      // The visitor will invoke this->StatisticsAlgorithm on data
      // from each node, placing its result into a local model.
      // Then it aggregates/inserts the local model into the \a model
      // this method was passed.
      assy->Visit(accumulator);
      didVisit = true;
#if VTK_DBG_ASSEMBLY
      {
        vtkIndent indent(2);
        std::cout << "Finished traverse of PDC and created models.\n"
                     "The model hierarchy is\n"
                  << modelTree->GetDataAssembly()->SerializeToXML(indent) << "\n";
      }
#endif
    }
  }
  if (!didVisit)
  {
    // There is no structure or (if !SingleModel) we are ignoring it;
    // just blob each model (i.e., each partitioned dataset) into its
    // own assembly-node at the root of the assembly.
    vtkNew<vtkDataAssembly> fakeAssembly;
    int numNodes = pdc->GetNumberOfPartitionedDataSets();
    if (this->SingleModel)
    {
      auto nodeId = fakeAssembly->AddNode("model");
      for (int ii = 0; ii < numNodes; ++ii)
      {
        fakeAssembly->AddDataSetIndex(nodeId, ii);
      }
    }
    else
    {
      for (int ii = 0; ii < numNodes; ++ii)
      {
        auto nodeId = fakeAssembly->AddNode(("model_" + vtk::to_string(ii)).c_str());
        fakeAssembly->AddDataSetIndex(nodeId, ii);
      }
    }
    // if (!this->SingleModel)
    {
      // Ensure the output model "hierarchy" has a matching structure:
      vtkNew<vtkDataAssembly> fakeCopy;
      fakeCopy->DeepCopy(fakeAssembly);
      fakeCopy->RemoveAllDataSetIndices(/*node*/ 0, /*recurse*/ true);
      modelTree->SetDataAssembly(fakeCopy);
    }
    fakeAssembly->Visit(accumulator);
  }
  if (this->SingleModel)
  {
    // Now that we've visited all the tree nodes, the accumulator's
    // CurrentModel should be a single model containing all the statistics
    // for the whole tree. Insert it into the tree.
    if (accumulator->CurrentModel)
    {
      modelTree->SetPartition(0, 0, accumulator->CurrentModel);
      // modelTree->GetMetaData((int)0)->Set(vtkCompositeDataSet::NAME(), "Statistics");
      vtkNew<vtkDataAssembly> singleModel;
      auto nodeId = singleModel->AddNode("Statistics", 0);
      singleModel->AddDataSetIndex(nodeId, 0);
      modelTree->SetDataAssembly(singleModel);
    }
  }
  return 1;
}

int vtkGenerateStatistics::RequestDataPD(vtkPartitionedDataSet* pd, vtkStatisticalModel* model)
{
  if (!model)
  {
    vtkErrorMacro("No output model.");
    return 0;
  }
  if (!pd)
  {
    // OK not to have data.
    return 1;
  }
  vtkNew<vtkStatisticalModel> localModel;
  vtkNew<vtkDataObjectCollection> models;
  for (unsigned int ii = 0; ii < pd->GetNumberOfPartitions(); ++ii)
  {
    localModel->Initialize();
    if (this->RequestDataNonComposite(pd->GetPartitionAsDataObject(ii), localModel) == 1)
    {
      // Aggregate into the global model for all partitions.
      models->RemoveAllItems();
      models->AddItem(localModel);
      this->GetStatisticsAlgorithm()->Aggregate(models, model);
    }
  }
  return 1;
}

int vtkGenerateStatistics::RequestDataCellGrid(vtkCellGrid* cellGrid, vtkStatisticalModel* model)
{
  if (!cellGrid)
  {
    return 1;
  }
  auto samples = vtkSmartPointer<vtkTable>::New();
  vtkNew<vtkCellGridSampleQuery> query;
  query->IncludeSourceCellInfoOff();
  query->IncludeSourceCellSiteOff();
  query->SetOutput(samples);
  query->SetInput(cellGrid);
  if (!cellGrid->Query(query))
  {
    vtkErrorMacro("Could not produce sample table.");
    return 0;
  }
  this->StatisticsAlgorithm->SetInputDataObject(vtkStatisticsAlgorithm::INPUT_DATA, samples);

  // Configure this->StatisticsAlgorithm for Learn mode
  // and run this->StatisticsAlgorithm
  this->StatisticsAlgorithm->SetLearnOption(true);
  this->StatisticsAlgorithm->SetDeriveOption(false);
  this->StatisticsAlgorithm->SetAssessOption(false);
  this->StatisticsAlgorithm->SetTestOption(false);
  this->StatisticsAlgorithm->Update();

  // Copy the computed model into the output model:
  model->DeepCopy(
    this->StatisticsAlgorithm->GetOutputDataObject(vtkStatisticsAlgorithm::OUTPUT_MODEL));
  return 1;
}

int vtkGenerateStatistics::RequestDataPlain(vtkDataObject* dataObject, vtkStatisticalModel* model)
{
  int allCenterings = 0;
  std::vector<int> centering;
  std::vector<vtkSmartPointer<vtkAbstractArray>> columns;
  bool ok = true;
  // Use the input array specifications to fetch the arrays specified for this dataObject.
  int numArrays = this->GetNumberOfInputArraySpecifications();
  for (int ii = 0; ii < numArrays; ++ii)
  {
    int association;
    auto array = this->GetInputArray(ii, dataObject, association, /*requestedComponent*/ 0);
    if (!array)
    {
      ok = false;
      break;
    }
    columns.push_back(array);
    centering.push_back(association);
    allCenterings |= (1 << association);
  }

  if (!ok)
  {
    // SILENTLY FAIL: Missing an array in request results in no model for this block.
    // This is not an error as this data object may be part of composite data that
    // has the requested arrays in other blocks.
    return 1;
  }

  double trainingFraction = this->TrainingFraction;
  constexpr int pointsBit = 1 << vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_POINTS;
  constexpr int cellsBit = 1 << vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_CELLS;
  constexpr int globalBit = 1 << vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_NONE;
  constexpr int rowsBit = 1 << vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_ROWS;
  constexpr int vertsBit = 1 << vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_VERTICES;
  constexpr int edgesBit = 1 << vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_EDGES;

  if ((allCenterings & (pointsBit | cellsBit)) && (allCenterings & (rowsBit | vertsBit | edgesBit)))
  {
    vtkErrorMacro(
      "Cannot combine point/cell and non-geometric associations in a single data-object.");
    return 0;
  }
  else if ((allCenterings & (rowsBit | globalBit)) && (allCenterings & ~(rowsBit | globalBit)))
  {
    vtkErrorMacro(
      "Cannot combine table data with other data (graph or geometric) in a single data-object.");
    return 0;
  }
  // clang-format off
  else if (
    (allCenterings & (edgesBit | vertsBit | globalBit)) &&
    (allCenterings & ~(edgesBit | vertsBit | globalBit)))
  {
    vtkErrorMacro("Cannot combine graph data with other data (tabular or geometric) in a single data-object.");
    return 0;
  }
  // clang-format on

  unsigned char ghostMask = 0;
  int sampleSpace = vtkDataObject::AttributeTypes::FIELD;
  std::unordered_map<vtkIdType, vtkIdType> subset; // list of tuples to keep from the input data
  // For geometric data with mixed centering, prefer resampling to points.
  if (allCenterings & pointsBit)
  {
    sampleSpace = vtkDataObject::AttributeTypes::POINT;
    ghostMask = vtkDataSetAttributes::PointGhostTypes::DUPLICATEPOINT |
      vtkDataSetAttributes::PointGhostTypes::HIDDENPOINT;
    // Do we have other data centered elsewhere?
    if (allCenterings & ~pointsBit)
    {
      // Choose a subset of non-ghosted points if trainingFraction < 1.
      if (trainingFraction < 1)
      {
        this->GenerateSubset(subset, dataObject->GetNumberOfElements(sampleSpace), trainingFraction,
          dataObject->GetAttributes(sampleSpace)->GetGhostArray(), ghostMask);
      }
      PointsOfCellsWeightMap cellsToPointsToWeights;
      if (allCenterings & cellsBit)
      {
        // Populate cellToPointsToWeight for use in array conversions below.
        this->ComputeCellToPointWeights(
          cellsToPointsToWeights, vtkDataSet::SafeDownCast(dataObject), subset);
      }
      // Convert cell- and global-data arrays to point-centered data via averaging
      // or weighted averaging, but only for points in the subset (if applicable).
      auto assocIt = centering.begin();
      for (auto& column : columns)
      {
        switch (*assocIt)
        {
          case vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_POINTS:
            column = this->SubsetArray(column, subset);
            break;
          case vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_CELLS:
            column = this->CellToPointSamples(
              column, vtkDataSet::SafeDownCast(dataObject), subset, cellsToPointsToWeights);
            break;
          case vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_NONE:
            column = this->FieldDataToSamples(
              column, dataObject, subset, dataObject->GetNumberOfElements(sampleSpace));
            break;
          default:
            vtkErrorMacro("Array \"" << column->GetName() << "\" with bad centering " << *assocIt
                                     << " compared to " << sampleSpace << ".");
            ok = false;
            break;
        }
        ++assocIt;
      }
    }
  }
  else if (allCenterings & cellsBit)
  {
    sampleSpace = vtkDataObject::AttributeTypes::CELL;
    ghostMask = vtkDataSetAttributes::CellGhostTypes::DUPLICATECELL |
      vtkDataSetAttributes::CellGhostTypes::REFINEDCELL |
      vtkDataSetAttributes::CellGhostTypes::HIDDENCELL;
    if (allCenterings & ~cellsBit)
    {
      // Choose a subset of non-ghosted cells if trainingFraction < 1.
      if (trainingFraction < 1.)
      {
        this->GenerateSubset(subset, dataObject->GetNumberOfElements(sampleSpace), trainingFraction,
          dataObject->GetAttributes(sampleSpace)->GetGhostArray(), ghostMask);
      }
      // Convert global-data arrays to cell-centered data via duplication,
      // but only for cells in the subset (if applicable).
      auto assocIt = centering.begin();
      for (auto& column : columns)
      {
        switch (*assocIt)
        {
          case vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_CELLS:
            column = this->SubsetArray(column, subset);
            break;
          case vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_NONE:
            column = this->FieldDataToSamples(
              column, dataObject, subset, dataObject->GetNumberOfElements(sampleSpace));
            break;
          default:
            vtkErrorMacro("Array \"" << column->GetName() << "\" with bad centering " << *assocIt
                                     << " compared to " << sampleSpace << ".");
            ok = false;
            break;
        }
        ++assocIt;
      }
    }
  }

  // For graph data with mixed centering, prefer resampling to vertices
  if (allCenterings & vertsBit)
  {
    sampleSpace = vtkDataObject::AttributeTypes::VERTEX;
    // TODO: We don't have ghost markings for graphs yet. Assume for now
    //       that if a graph is distributed, we will use point markings
    //       for graph vertices.
    ghostMask = vtkDataSetAttributes::PointGhostTypes::DUPLICATEPOINT |
      vtkDataSetAttributes::PointGhostTypes::HIDDENPOINT;
    if (allCenterings & ~vertsBit)
    {
      // Choose a subset of non-ghosted vertices if trainingFraction < 1.
      if (trainingFraction < 1)
      {
        this->GenerateSubset(subset, dataObject->GetNumberOfElements(sampleSpace), trainingFraction,
          dataObject->GetAttributes(sampleSpace)->GetGhostArray(), ghostMask);
      }
      std::unordered_map<vtkIdType, std::unordered_map<vtkIdType, double>> edgesToVertsToWeights;
      if (allCenterings & edgesBit)
      {
        // Populate edgesToVertsToWeight for use in array conversions below.
        this->ComputeEdgeToVertexWeights(
          edgesToVertsToWeights, vtkGraph::SafeDownCast(dataObject), subset);
      }
      // Convert edge- and global-data arrays to vertex-centered data via averaging or weighted
      // averaging, but only for vertices in the subset (if applicable).
      auto assocIt = centering.begin();
      for (auto& column : columns)
      {
        switch (*assocIt)
        {
          case vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_VERTICES:
            column = this->SubsetArray(column, subset);
            break;
          case vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_EDGES:
            column = this->EdgeToVertexSamples(
              column, vtkGraph::SafeDownCast(dataObject), subset, edgesToVertsToWeights);
            break;
          case vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_NONE:
            column = this->FieldDataToSamples(
              column, dataObject, subset, dataObject->GetNumberOfElements(sampleSpace));
            break;
          default:
            vtkErrorMacro("Array \"" << column->GetName() << "\" with bad centering " << *assocIt
                                     << " compared to " << sampleSpace << ".");
            ok = false;
            break;
        }
        ++assocIt;
      }
    }
  }
  else if (allCenterings & edgesBit)
  {
    sampleSpace = vtkDataObject::AttributeTypes::EDGE;
    // TODO: We don't have ghost markings for graphs yet. Assume for now
    //       that if a graph is distributed, we will use point markings
    //       for graph edges.
    ghostMask = vtkDataSetAttributes::CellGhostTypes::DUPLICATECELL |
      vtkDataSetAttributes::CellGhostTypes::REFINEDCELL |
      vtkDataSetAttributes::CellGhostTypes::HIDDENCELL;
    if (allCenterings & ~edgesBit)
    {
      // Choose a subset of non-ghosted edges if trainingFraction < 1.
      if (trainingFraction < 1)
      {
        this->GenerateSubset(subset, dataObject->GetNumberOfElements(sampleSpace), trainingFraction,
          dataObject->GetAttributes(sampleSpace)->GetGhostArray(), ghostMask);
      }
      // Convert global-data arrays to edge-centered data via duplication,
      // but only for edges in the subset (if applicable).
      auto assocIt = centering.begin();
      for (auto& column : columns)
      {
        switch (*assocIt)
        {
          case vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_EDGES:
            column = this->SubsetArray(column, subset);
            break;
          case vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_NONE:
            column = this->FieldDataToSamples(
              column, dataObject, subset, dataObject->GetNumberOfElements(sampleSpace));
            break;
          default:
            vtkErrorMacro("Array \"" << column->GetName() << "\" with bad centering " << *assocIt
                                     << " compared to " << sampleSpace << ".");
            ok = false;
            break;
        }
        ++assocIt;
      }
    }
  }

  // For tabular data with mixed centering, resample to rows (i.e., duplicate global data)
  if (allCenterings & rowsBit)
  {
    sampleSpace = vtkDataObject::AttributeTypes::ROW;
    // TODO: We don't have ghost markings for tables yet. Assume for now
    //       that if a table is distributed, we will use point markings
    //       for table rows.
    ghostMask = vtkDataSetAttributes::PointGhostTypes::DUPLICATEPOINT |
      vtkDataSetAttributes::PointGhostTypes::HIDDENPOINT;
    if (allCenterings & ~rowsBit)
    {
      // Choose a subset of non-ghosted rows if trainingFraction < 1.
      if (trainingFraction < 1 && subset.empty())
      {
        // Choose a subset of non-ghosted rows
        this->GenerateSubset(subset, dataObject->GetNumberOfElements(sampleSpace), trainingFraction,
          dataObject->GetAttributes(sampleSpace)->GetGhostArray(), ghostMask);
      }
      // Convert global-data arrays to row-centered data via duplication
      auto assocIt = centering.begin();
      for (auto& column : columns)
      {
        switch (*assocIt)
        {
          case vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_ROWS:
            column = this->SubsetArray(column, subset);
            break;
          case vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_NONE:
            column = this->FieldDataToSamples(
              column, dataObject, subset, dataObject->GetNumberOfElements(sampleSpace));
            break;
          default:
            vtkErrorMacro("Array \"" << column->GetName() << "\" with bad centering " << *assocIt
                                     << " compared to " << sampleSpace << ".");
            ok = false;
            break;
        }
        ++assocIt;
      }
    }
  }

  if (allCenterings & globalBit)
  {
    // We only get here if all the arrays selected are global data.
    // We better only have one sample.
    subset.clear();
    // TODO: We could allow users to specify the tuple ID of field data array
    //       they wish to process. For now, just ensure we take only the first
    //       tuple of each column.
    subset[0] = 0;
    auto assocIt = centering.begin();
    for (auto& column : columns)
    {
      assert(*assocIt == vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_NONE);
      if (column->GetNumberOfTuples() > 1)
      {
        column = this->SubsetArray(column, subset);
      }
      ++assocIt;
    }
  }

  // The table to hold all the column data.
  vtkNew<vtkTable> data;

  // If no arrays needed resampling but trainingFraction < 1., we need to
  // choose a subset of the arrays here.
  if (trainingFraction < 1 && subset.empty())
  {
    // Choose a subset of non-ghosted rows
    this->GenerateSubset(subset, dataObject->GetNumberOfElements(sampleSpace), trainingFraction,
      dataObject->GetAttributes(sampleSpace)->GetGhostArray(), ghostMask);
    // Loop over columns which we know have the same centering (association)
    // and choose the subset.
    auto assocIt = centering.begin();
    for (auto& column : columns)
    {
      if (*assocIt == sampleSpace)
      {
        column = this->SubsetArray(column, subset);
      }
      else
      {
        vtkErrorMacro("Array \"" << column->GetName() << "\" with bad centering " << *assocIt
                                 << " compared to " << sampleSpace << ".");
        ok = false;
      }
      ++assocIt;
    }
  }
  else if (subset.empty())
  {
    // We are not subsetting and all the arrays live in \a sampleSpace
    // now (i.e., they should all have the same number of tuples).
    // Because we are not subsetting (which will pay attention to ghost
    // markings), if we have ghost markings in our sample space we must
    // add them to the table.
    data->GetRowData()->AddArray(dataObject->GetAttributes(sampleSpace)->GetGhostArray());
  }

  if (!ok)
  {
    // We had all the requested arrays but couldn't generate a table for
    // some reason. This is an error.
    return 0;
  }

  // Populate the data table with \a columns
  for (const auto& column : columns)
  {
    data->AddColumn(column);
  }
  // Set up the algorithm with the requested columns (or with
  // multiple requests if the number of columns is larger than
  // the number allowed per request).
  this->PrepareAlgorithmRequests(columns);
#if VTK_DBG_INPUTDATA
  {
    std::ostringstream fname;
    int rank = this->Controller->GetLocalProcessId();
    fname << "modelData_" << runId++ << "_" << rank << ".dat";
    std::ofstream foo(fname.str().c_str());

    foo << "--- " << rank << " of " << this->Controller->GetNumberOfProcesses() << " ---\n";
    DumpTo(foo, data, 20, -1);
    foo.close();
  }
#endif
  this->StatisticsAlgorithm->SetInputDataObject(vtkStatisticsAlgorithm::INPUT_DATA, data);

  // Configure this->StatisticsAlgorithm for Learn mode
  // and run this->StatisticsAlgorithm
  this->StatisticsAlgorithm->SetLearnOption(true);
  this->StatisticsAlgorithm->SetDeriveOption(false);
  this->StatisticsAlgorithm->SetAssessOption(false);
  this->StatisticsAlgorithm->SetTestOption(false);
  this->StatisticsAlgorithm->Update();

  // Copy the computed model into the output model:
  model->DeepCopy(
    this->StatisticsAlgorithm->GetOutputDataObject(vtkStatisticsAlgorithm::OUTPUT_MODEL));
  return 1;
}

void vtkGenerateStatistics::PrepareAlgorithmRequests(
  const std::vector<vtkSmartPointer<vtkAbstractArray>>& columns)
{
  int mm = static_cast<int>(columns.size());
  int nn = this->StatisticsAlgorithm->GetMaximumNumberOfColumnsPerRequest();
  this->StatisticsAlgorithm->ResetRequests();
  if (nn > 0 && mm > nn)
  {
    auto* combo = vtkMath::BeginCombination(mm, nn);
    for (int more = 1; more && combo; more = vtkMath::NextCombination(mm, nn, combo))
    {
      this->StatisticsAlgorithm->ResetAllColumnStates();
      for (int ii = 0; ii < nn; ++ii)
      {
        this->StatisticsAlgorithm->SetColumnStatus(columns[combo[ii]]->GetName(), true);
      }
      this->StatisticsAlgorithm->RequestSelectedColumns();
    }
    vtkMath::FreeCombination(combo);
  }
  else
  {
    this->StatisticsAlgorithm->ResetAllColumnStates();
    for (const auto& column : columns)
    {
      this->StatisticsAlgorithm->SetColumnStatus(column->GetName(), true);
    }
    this->StatisticsAlgorithm->RequestSelectedColumns();
  }
}

vtkIdType vtkGenerateStatistics::GetNumberOfObservationsForTraining(vtkIdType N)
{
  vtkIdType M = static_cast<vtkIdType>(N * this->TrainingFraction);
  return std::clamp(M, static_cast<vtkIdType>(100), M);
}

void vtkGenerateStatistics::ShallowCopy(vtkDataObject* out, vtkDataObject* in)
{
  // Our output is always composite:
  vtkCompositeDataSet* cdOut = vtkCompositeDataSet::SafeDownCast(out);
  // Use a different method to copy the input if the input is composite
  // so that leaf nodes are not simply references to the input data
  // (since we may modify them).
  if (auto cdIn = vtkCompositeDataSet::SafeDownCast(in))
  {
    cdOut->CompositeShallowCopy(cdIn);
  }
  else
  {
    out->ShallowCopy(in);
  }
}

void vtkGenerateStatistics::GenerateSubset(std::unordered_map<vtkIdType, vtkIdType>& subset,
  vtkIdType numberOfSamples, double trainingFraction, vtkUnsignedCharArray* ghostData,
  unsigned char ghostMask)
{
  if (!ghostData)
  {
    vtkReservoirSampler<vtkIdType, /* monotonic output */ false> sampler;
    std::vector<vtkIdType> ids = sampler(numberOfSamples * trainingFraction, numberOfSamples);
    vtkIdType out = 0;
    for (const auto& id : ids)
    {
      subset[id] = out;
      ++out;
    }
  }
  else
  {
    vtkIdType actualNumberOfSamples = 0;
    // Count non-ghost values:
    for (vtkIdType ii = 0; ii < ghostData->GetNumberOfValues(); ++ii)
    {
      if ((ghostData->GetValue(ii) & ghostMask) == 0)
      {
        ++actualNumberOfSamples;
      }
    }
    // Compute indices as if ghost values were not present:
    vtkReservoirSampler<vtkIdType, /* monotonic output */ true> sampler;
    std::vector<vtkIdType> ids =
      sampler(actualNumberOfSamples * trainingFraction, actualNumberOfSamples);
    // Compute actual sample indices by skipping ghosts:
    vtkIdType virtualIndex = 0;
    auto idIt = ids.begin();
    vtkIdType out = 0;
    for (vtkIdType ii = 0; ii < ghostData->GetNumberOfValues(); ++ii)
    {
      if ((ghostData->GetValue(ii) & ghostMask) == 0)
      {
        if (*idIt == virtualIndex)
        {
          subset[ii] = out++;
          ++idIt;
          if (idIt == ids.end())
          {
            // Terminate early; we have all our samples.
            break;
          }
        }
        ++virtualIndex;
      }
    }
  }
}

void vtkGenerateStatistics::ComputeCellToPointWeights(
  PointsOfCellsWeightMap& cellsToPointsToWeights, vtkDataSet* dataSet,
  const std::unordered_map<vtkIdType, vtkIdType>& subset)
{
  vtkNew<vtkIdList> stupid;
  vtkSmartPointer<vtkDataArray> weights;
  if (this->WeightByCellMeasure)
  {
    vtkNew<vtkCellSizeFilter> computeMeasure;
    computeMeasure->SetInputDataObject(0, dataSet);
    computeMeasure->ComputeLengthOn();
    computeMeasure->ComputeAreaOn();
    computeMeasure->ComputeVolumeOn();
    computeMeasure->SetLengthArrayName("measure");
    computeMeasure->SetAreaArrayName("measure");
    computeMeasure->SetVolumeArrayName("measure");
    computeMeasure->Update();
    weights =
      vtkDataSet::SafeDownCast(computeMeasure->GetOutput(0))->GetCellData()->GetArray("measure");
  }
  if (subset.empty())
  {
    // Compute all cell-to-point weights
    vtkIdType numberOfCells = dataSet->GetNumberOfCells();
    vtkIdType npts;
    vtkIdType const* conn;
    for (vtkIdType cc = 0; cc < numberOfCells; ++cc)
    {
      dataSet->GetCellPoints(cc, npts, conn, stupid);
      double weight = 1. / npts;
      if (this->WeightByCellMeasure)
      {
        weights->GetTuple(cc, &weight);
        weight = weight / npts;
      }
      for (vtkIdType jj = 0; jj < npts; ++jj)
      {
        cellsToPointsToWeights[cc][conn[jj]] = weight;
      }
    }
  }
  else
  {
    // Compute only weights for point IDs listed in subset.
    double weight;
    for (const auto& entry : subset)
    {
      auto pointId = entry.first;
      dataSet->GetPointCells(pointId, stupid);
      for (const auto& cellId : *stupid)
      {
        auto npts = dataSet->GetCellSize(cellId);
        if (this->WeightByCellMeasure)
        {
          weights->GetTuple(cellId, &weight);
          weight = weight / npts;
        }
        else
        {
          weight = 1. / npts;
        }
        cellsToPointsToWeights[cellId][entry.second] = weight;
      }
    }
  }
}

void vtkGenerateStatistics::ComputeEdgeToVertexWeights(
  std::unordered_map<vtkIdType, std::unordered_map<vtkIdType, double>>& vtkNotUsed(
    edgesToVertsToWeights),
  vtkGraph* vtkNotUsed(graph), const std::unordered_map<vtkIdType, vtkIdType>& vtkNotUsed(subset))
{
  vtkErrorMacro("Graph statistics combining edge and vertex data are unsupported.");
}

vtkSmartPointer<vtkAbstractArray> vtkGenerateStatistics::SubsetArray(
  vtkSmartPointer<vtkAbstractArray> fullArray,
  const std::unordered_map<vtkIdType, vtkIdType>& subset)
{
  if (subset.empty())
  {
    return fullArray;
  }
  vtkSmartPointer<vtkAbstractArray> array;
  array.TakeReference(vtkAbstractArray::CreateArray(fullArray->GetDataType()));
  array->SetNumberOfTuples(static_cast<vtkIdType>(subset.size()));
  array->SetName(fullArray->GetName());
  for (const auto& entry : subset)
  {
    auto tupleId = entry.first;
    auto out = entry.second;
    array->SetTuple(out, tupleId, fullArray);
  }
  return array;
}

vtkSmartPointer<vtkAbstractArray> vtkGenerateStatistics::CellToPointSamples(
  vtkSmartPointer<vtkAbstractArray> fullArray, vtkDataSet* data,
  const std::unordered_map<vtkIdType, vtkIdType>& subset,
  const PointsOfCellsWeightMap& cellsToPointsToWeights)
{
  auto* cellArray = vtkDataArray::SafeDownCast(fullArray);
  if (!cellArray)
  {
    vtkErrorMacro("Converting " << fullArray->GetClassName() << " named \"" << fullArray->GetName()
                                << "\" from "
                                   "cell-centered to point-centered is unsupported.");
    return nullptr;
  }
  vtkSmartPointer<vtkDataArray> result;
  result.TakeReference(cellArray->NewInstance());
  result->SetNumberOfTuples(
    subset.empty() ? data->GetNumberOfPoints() : static_cast<vtkIdType>(subset.size()));
  result->FillComponent(0, 0.);
  result->SetName(cellArray->GetName());
  // Create an array to hold the sum of the weights for each point.
  // This is used to normalize the output point values.
  vtkNew<vtkDoubleArray> weightSum;
  weightSum->SetName("weightSum");
  weightSum->SetNumberOfTuples(
    subset.empty() ? data->GetNumberOfPoints() : static_cast<vtkIdType>(subset.size()));
  weightSum->FillComponent(0, 0.);
  double cellValue;
  double resultValue;
  double currentWeight;
  // Since cellsToPointsToWeights only contains entries for
  // points in the \a subset (if \a subset is non-empty) and
  // contains values for all points (if \a subset is empty),
  // we can just loop over cellsToPointsToWeights to splat
  // exactly what is needed.
  for (const auto& entry : cellsToPointsToWeights)
  {
    auto cellId = entry.first;
    cellArray->GetTuple(cellId, &cellValue);
    for (const auto& pointToWeight : entry.second)
    {
      result->GetTuple(pointToWeight.first, &resultValue);
      resultValue += pointToWeight.second * cellValue;
      result->SetTuple(pointToWeight.first, &resultValue);
      weightSum->GetTuple(pointToWeight.first, &currentWeight);
      currentWeight += pointToWeight.second;
      weightSum->SetTuple(pointToWeight.first, &currentWeight);
    }
  }
  // Now divide each point's value by its matching weightSum.
  // This also conveniently turns points with no contribution from any cell into NaN values
  // for us. In the future we might offer users an option to replace NaN values with some
  // meaningful constant on a per-array basis.
  vtkSMPTools::For(0, result->GetNumberOfTuples(),
    [&](vtkIdType begin, vtkIdType end)
    {
      double vv;
      double ww;
      for (vtkIdType ii = begin; ii < end; ++ii)
      {
        result->GetTuple(ii, &vv);
        weightSum->GetTuple(ii, &ww);
        vv = vv / ww;
        result->SetTuple(ii, &vv);
      }
    });
  return result;
}

vtkSmartPointer<vtkAbstractArray> vtkGenerateStatistics::EdgeToVertexSamples(
  vtkSmartPointer<vtkAbstractArray> vtkNotUsed(fullArray), vtkGraph* vtkNotUsed(data),
  const std::unordered_map<vtkIdType, vtkIdType>& vtkNotUsed(subset),
  const std::unordered_map<vtkIdType, std::unordered_map<vtkIdType, double>>& vtkNotUsed(
    edgesToVertsToWeights))
{
  // TODO
  vtkErrorMacro("Resampling graph edges to vertices is not yet supported.");
  return nullptr;
}

vtkSmartPointer<vtkAbstractArray> vtkGenerateStatistics::FieldDataToSamples(
  vtkSmartPointer<vtkAbstractArray> fullArray, vtkDataObject* vtkNotUsed(data),
  const std::unordered_map<vtkIdType, vtkIdType>& subset, vtkIdType numberOfSamples)
{
  auto* dataArray = vtkDataArray::SafeDownCast(fullArray);
  if (!dataArray)
  {
    vtkErrorMacro("Duplicate field data must be passed vtkDataArrays.");
    return vtkSmartPointer<vtkAbstractArray>();
  }
  vtkIdType tableSize = subset.empty() ? numberOfSamples : static_cast<vtkIdType>(subset.size());
  vtkSmartPointer<vtkConstantArray<double>> arr = vtkSmartPointer<vtkConstantArray<double>>::New();
  double value;
  dataArray->GetTuple(0, &value);
  arr->SetBackend(std::make_shared<vtkConstantImplicitBackend<double>>(value));
  arr->SetNumberOfComponents(1);
  arr->SetNumberOfTuples(tableSize);
  arr->SetName(dataArray->GetName());

  return arr;
}

int vtkGenerateStatistics::MergeRemoteModels(vtkPartitionedDataSetCollection* modelTree)
{
  auto* controller = this->Controller;
#if VTK_DBG_MODELDATA
  reportRanks("*** Before Merge ***", modelTree, controller);
#endif
  if (!controller)
  {
    controller = vtkMultiProcessController::GetGlobalController();
  }
  if (!controller)
  {
    // No work to do.
    return 0;
  }
  int rank = controller->GetLocalProcessId();
  int numberOfRanks = controller->GetNumberOfProcesses();
  if (numberOfRanks < 2)
  {
    // No work to do.
    return 0;
  }
  int maxPower = std::log2(vtkMath::NearestPowerOfTwo(numberOfRanks));
#if VTK_DBG_MODELDATA
  // For debugging:
  std::ostringstream commlog;
  commlog << "/tmp/comm_" << rank << ".log";
#endif
  // Create a writer to serialize **LOCAL** model information ONLY.
  // We send the resulting XML via \a controller to a partner rank per pass,
  // where it will be aggregated with the partner's model (if any).
  vtkNew<vtkGenericDataObjectWriter> serializer;
  serializer->SetInputDataObject(0, modelTree);
  serializer->WriteToOutputStringOn();
  serializer->Write();
  std::string localModelString = serializer->GetOutputString();
  std::size_t localModelSize = localModelString.size();
  vtkNew<vtkGenericDataObjectReader> deserializer;
  deserializer->ReadFromInputStringOn();
  {
#if VTK_DBG_MODELDATA
    std::ofstream log(commlog.str().c_str());
    log << "max power " << maxPower << "\n";
#endif
    for (int power = 0; power < maxPower; ++power)
    {
#if VTK_DBG_MODELDATA
      log << "Pass " << power << "\n";
#endif
      int delta = (1 << power);
      if (rank % delta == 0)
      {
        bool recvUp = (rank % (2 * delta) == 0);
        if (recvUp)
        {
          int recvFrom = (rank + delta >= numberOfRanks ? -1 : rank + delta);
          if (recvFrom < 0)
          {
#if VTK_DBG_MODELDATA
            log << "  " << rank << " skip\n";
#endif
          }
          else
          {
#if VTK_DBG_MODELDATA
            log << "  " << rank << " receive from " << recvFrom << "\n";
#endif
            std::size_t remoteModelSize;
            controller->Receive(&remoteModelSize, 1, recvFrom, /*tag*/ 128);
            std::string remoteModelString;
            remoteModelString.resize(remoteModelSize);
            controller->Receive(remoteModelString.data(), remoteModelSize, recvFrom, /*tag*/ 128);
            deserializer->SetInputString(
              remoteModelString.data(), static_cast<int>(remoteModelString.size()));
            deserializer->Update();
            this->MergeModelTrees(
              vtkPartitionedDataSetCollection::SafeDownCast(deserializer->GetOutputDataObject(0)),
              modelTree);
            serializer->SetInputDataObject(0, modelTree);
            serializer->Write();
            localModelString = serializer->GetOutputString();
            localModelSize = localModelString.size();
          }
        }
        else
        {
          int sendTo = (rank == 0 ? -1 : rank - delta);
          if (sendTo < 0)
          {
#if VTK_DBG_MODELDATA
            log << "  " << rank << " skip\n";
#endif
          }
          else
          {
#if VTK_DBG_MODELDATA
            log << "  " << rank << " send to " << sendTo << "\n";
#endif
            controller->Send(&localModelSize, 1, sendTo, /*tag*/ 128);
            controller->Send(localModelString.data(), localModelString.size(), sendTo, /*tag*/ 128);
          }
        }
      }
      else
      {
#if VTK_DBG_MODELDATA
        log << "  " << rank << " skip\n";
#endif
      }
    }
#if VTK_DBG_MODELDATA
    log << "  " << rank << " broadcast from rank 0\n";
#endif
    controller->Broadcast(&localModelSize, 1, 0);
    if (rank != 0)
    {
      localModelString.resize(localModelSize);
    }
    controller->Broadcast(localModelString.data(), localModelSize, 0);
    if (rank != 0)
    {
      deserializer->SetInputString(
        localModelString.data(), static_cast<int>(localModelString.size()));
      deserializer->Update();
      modelTree->ShallowCopy(deserializer->GetOutputDataObject(0));
    }
#if VTK_DBG_MODELDATA
    log << localModelString << "\n";
    log.close();
#endif
  }
#if VTK_DBG_MODELDATA
  reportRanks("*** After Merge ***", modelTree, controller);
#endif
  return 1;
}

int vtkGenerateStatistics::MergeModelTrees(
  vtkPartitionedDataSetCollection* other, vtkPartitionedDataSetCollection* target)
{
  vtkNew<vtkModelMerger> mergeVisitor;
  mergeVisitor->SourceData = other;
  mergeVisitor->TargetData = target;
  mergeVisitor->SourceAssembly = other->GetDataAssembly();
  mergeVisitor->TargetAssembly = target->GetDataAssembly();
  mergeVisitor->Algorithm = this->StatisticsAlgorithm;
  mergeVisitor->TargetAssembly->Visit(mergeVisitor);
  mergeVisitor->Self = this;
  return 1;
}

int vtkGenerateStatistics::ComputeDerivedData(vtkPartitionedDataSetCollection* model)
{
  // Iterate over the statistical models in the PDC and add derived
  // statistics to them.
  int numPDs = model->GetNumberOfPartitionedDataSets();
  for (int ii = 0; ii < numPDs; ++ii)
  {
    auto* pds = model->GetPartitionedDataSet(ii);
    for (unsigned int jj = 0; jj < pds->GetNumberOfPartitions(); ++jj)
    {
      if (auto* stats = vtkStatisticalModel::SafeDownCast(pds->GetPartitionAsDataObject(jj)))
      {
        // Configure this->StatisticsAlgorithm for Learn mode
        // and run this->StatisticsAlgorithm
        this->StatisticsAlgorithm->SetInputDataObject(vtkStatisticsAlgorithm::INPUT_MODEL, stats);
        this->StatisticsAlgorithm->SetLearnOption(false);
        this->StatisticsAlgorithm->SetDeriveOption(true);
        this->StatisticsAlgorithm->SetAssessOption(false);
        this->StatisticsAlgorithm->SetTestOption(false);
        this->StatisticsAlgorithm->Update();

        // Copy or merge the model output into the given PDC
        // TODO: Do this better (match structure, look at SingleModel ivar)
        stats->ShallowCopy(
          this->StatisticsAlgorithm->GetOutputDataObject(vtkStatisticsAlgorithm::OUTPUT_MODEL));
      }
    }
  }
  return 1;
}

VTK_ABI_NAMESPACE_END
