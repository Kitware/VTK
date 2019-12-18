/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractDataArraysOverTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractDataArraysOverTime.h"

#include "vtkArrayDispatch.h"
#include "vtkCharArray.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDescriptiveStatistics.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOrderStatistics.h"
#include "vtkSmartPointer.h"
#include "vtkSplitColumnComponents.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"
#include "vtkWeakPointer.h"

#include <algorithm>
#include <cassert>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace
{
struct ClearInvalidElementsWorker
{
private:
  vtkCharArray* MaskArray;

public:
  ClearInvalidElementsWorker(vtkCharArray* maskArray)
    : MaskArray(maskArray)
  {
  }

  template <typename ArrayType>
  void operator()(ArrayType* vtkarray)
  {
    const auto mask = vtk::DataArrayValueRange<1>(this->MaskArray);
    auto data = vtk::DataArrayTupleRange(vtkarray);

    for (vtkIdType t = 0; t < data.size(); ++t)
    {
      if (mask[t] == 0)
      {
        data[t].fill(0);
      }
    }
  }
};
}

class vtkExtractDataArraysOverTime::vtkInternal
{
private:
  class vtkKey
  {
  public:
    unsigned int CompositeID;
    vtkIdType ID;

    vtkKey(vtkIdType id)
    {
      this->CompositeID = 0;
      this->ID = id;
    }
    vtkKey(unsigned int cid, vtkIdType id)
    {
      this->CompositeID = cid;
      this->ID = id;
    }

    bool operator<(const vtkKey& other) const
    {
      if (this->CompositeID == other.CompositeID)
      {
        return (this->ID < other.ID);
      }
      return (this->CompositeID < other.CompositeID);
    }
  };

public:
  class vtkValue
  {
  public:
    vtkSmartPointer<vtkTable> Output;
    vtkSmartPointer<vtkCharArray> ValidMaskArray;
    vtkSmartPointer<vtkDoubleArray> PointCoordinatesArray;
    bool UsingGlobalIDs;
    vtkValue()
      : UsingGlobalIDs(false)
    {
    }
  };

private:
  typedef std::map<vtkKey, vtkValue> MapType;
  MapType OutputGrids;
  int NumberOfTimeSteps;
  vtkWeakPointer<vtkExtractDataArraysOverTime> Self;
  // We use the same time array for all extracted time lines, since that doesn't
  // change.
  vtkSmartPointer<vtkDoubleArray> TimeArray;

  void AddTimeStepInternal(unsigned int cid, int ts_index, double time, vtkDataObject* data);

  /**
   * Runs stats filters to summarize the data and return
   * a new dataobject with the summary.
   */
  vtkSmartPointer<vtkDataObject> Summarize(vtkDataObject* data);

  vtkValue* GetOutput(const vtkKey& key, vtkDataSetAttributes* inDSA, bool using_gid);

  // For all arrays in dsa, for any element that's not valid (i.e. has value 1
  // in validArray), we initialize that element to 0 (rather than having some
  // garbage value).
  void RemoveInvalidPoints(vtkCharArray* validArray, vtkDataSetAttributes* dsa)
  {
    ClearInvalidElementsWorker worker(validArray);
    const auto narrays = dsa->GetNumberOfArrays();
    for (vtkIdType a = 0; a < narrays; a++)
    {
      if (vtkDataArray* da = dsa->GetArray(a))
      {
        if (!vtkArrayDispatch::Dispatch::Execute(da, worker))
        {
          // use vtkDataArray fallback.
          worker(da);
        }
      }
    }
  }

public:
  // Initializes the data structure.
  vtkInternal(int numTimeSteps, vtkExtractDataArraysOverTime* self)
    : NumberOfTimeSteps(numTimeSteps)
    , Self(self)
  {
    this->TimeArray = vtkSmartPointer<vtkDoubleArray>::New();
    this->TimeArray->SetNumberOfTuples(this->NumberOfTimeSteps);
    std::fill_n(
      this->TimeArray->WritePointer(0, this->NumberOfTimeSteps), this->NumberOfTimeSteps, 0.0);
    this->OutputGrids.clear();
  }

  void AddTimeStep(int ts_index, double time, vtkDataObject* data);

  // Collect the gathered timesteps into the output.
  void CollectTimesteps(vtkDataObject* input, vtkMultiBlockDataSet* mboutput)
  {
    assert(mboutput);

    mboutput->Initialize();

    // for now, let's not use blocknames. Seems like they are not consistent
    // across ranks currently. that makes it harder to merge blocks using
    // names in vtkPExtractDataArraysOverTime.
    (void)input;
#if 0
    auto mbinput = vtkCompositeDataSet::SafeDownCast(input);

    // build a datastructure to make block-name lookup fast.
    std::map<unsigned int, std::string> block_names;
    if (mbinput)
    {
      vtkCompositeDataIterator* iter = mbinput->NewIterator();
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        if (iter->HasCurrentMetaData() &&
          iter->GetCurrentMetaData()->Has(vtkCompositeDataSet::NAME()))
        {
          block_names[iter->GetCurrentFlatIndex()] =
            iter->GetCurrentMetaData()->Get(vtkCompositeDataSet::NAME());
        }
      }
      iter->Delete();
    }
#endif

    unsigned int cc = 0;
    for (auto& item : this->OutputGrids)
    {
      const vtkKey& key = item.first;
      const vtkValue& value = item.second;
      if (value.Output == nullptr)
      {
        continue;
      }
      auto outputRD = value.Output->GetRowData();

      vtkSmartPointer<vtkDataArray> originalIdsArray = nullptr;
      if (!this->Self->GetReportStatisticsOnly())
      {
        std::string originalIdsArrayName = "vtkOriginalCellIds";
        if (this->Self->GetFieldAssociation() == vtkDataObject::POINT)
        {
          originalIdsArrayName = "vtkOriginalPointIds";
        }
        originalIdsArray = outputRD->GetArray(originalIdsArrayName.c_str());
        // Remove vtkOriginalCellIds or vtkOriginalPointIds arrays which were added by
        // vtkExtractSelection.
        outputRD->RemoveArray(originalIdsArrayName.c_str());
      }

      outputRD->RemoveArray(value.ValidMaskArray->GetName());
      outputRD->AddArray(value.ValidMaskArray);
      if (value.PointCoordinatesArray)
      {
        outputRD->RemoveArray(value.PointCoordinatesArray->GetName());
        outputRD->AddArray(value.PointCoordinatesArray);
      }
      this->RemoveInvalidPoints(value.ValidMaskArray, outputRD);
      // note: don't add time array before the above step to avoid clearing
      // time values entirely.
      outputRD->RemoveArray(this->TimeArray->GetName());
      outputRD->AddArray(this->TimeArray);

      mboutput->SetBlock(cc, value.Output);

      // build a good name for the block.
      std::ostringstream stream;

      // add element id if not reporting stats.
      if (!this->Self->GetReportStatisticsOnly())
      {
        if (value.UsingGlobalIDs)
        {
          stream << "gid=" << key.ID;
        }
        else if (originalIdsArray)
        {
          stream << "originalId=" << originalIdsArray->GetTuple1(0);
        }
        else
        {
          stream << "id=" << key.ID;
        }
      }
      if (key.CompositeID != 0)
      {
        // for now, let's not use blocknames. Seems like they are not consistent
        // across ranks currently. that makes it harder to merge blocks using
        // names in vtkPExtractDataArraysOverTime.
#if 0
        auto iter = block_names.find(key.CompositeID);
        if (iter != block_names.end())
        {
          stream << " block=" << iter->second;
        }
        else
#endif
        {
          stream << " block=" << key.CompositeID;
        }
      }
      else if (stream.str().empty())
      {
        assert(this->Self->GetReportStatisticsOnly());
        stream << "stats";
      }
      mboutput->GetMetaData(cc)->Set(vtkCompositeDataSet::NAME(), stream.str().c_str());
      cc++;
    }
    this->OutputGrids.clear();
  }
};

//----------------------------------------------------------------------------
void vtkExtractDataArraysOverTime::vtkInternal::AddTimeStep(
  int ts_index, double time, vtkDataObject* data)
{
  this->TimeArray->SetTypedComponent(ts_index, 0, time);
  const int attributeType = this->Self->GetFieldAssociation();

  if (auto cd = vtkCompositeDataSet::SafeDownCast(data))
  {
    vtkCompositeDataIterator* iter = cd->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      if (auto block = iter->GetCurrentDataObject())
      {
        if (block->GetAttributesAsFieldData(attributeType) != nullptr)
        {
          this->AddTimeStepInternal(iter->GetCurrentFlatIndex(), ts_index, time, block);
        }
      }
    }
    iter->Delete();
  }
  else if (data)
  {
    if (data->GetAttributesAsFieldData(attributeType) != nullptr)
    {
      this->AddTimeStepInternal(0, ts_index, time, data);
    }
  }
}

//----------------------------------------------------------------------------
static void vtkExtractArraysAssignUniqueCoordNames(
  vtkDataSetAttributes* statInDSA, vtkDataArray* px, vtkDataArray* py, vtkDataArray* pz)
{
  std::string actualNames[3];
  actualNames[0] = "X";
  actualNames[1] = "Y";
  actualNames[2] = "Z";
  // We need to find unique but consistent names as close to
  // ("X","Y","Z") as possible, but that aren't in use.
  vtkAbstractArray* arrX;
  vtkAbstractArray* arrY;
  vtkAbstractArray* arrZ;
  int counter = 0;
  while ((arrX = statInDSA->GetArray(actualNames[0].c_str())) != nullptr &&
    (arrY = statInDSA->GetArray(actualNames[1].c_str())) != nullptr &&
    (arrZ = statInDSA->GetArray(actualNames[2].c_str())) != nullptr)
  {
    for (int i = 0; i < 3; ++i)
    {
      std::ostringstream os;
      os << "SelnCoords" << counter << "_" << (i ? (i > 1 ? "Z" : "Y") : "X");
      actualNames[i] = os.str();
    }
    ++counter;
  }
  px->SetName(actualNames[0].c_str());
  py->SetName(actualNames[1].c_str());
  pz->SetName(actualNames[2].c_str());
  statInDSA->AddArray(px);
  statInDSA->AddArray(py);
  statInDSA->AddArray(pz);
}

//------------------------------------------------------------------------------
static void vtkExtractArraysAddColumnValue(
  vtkTable* statSummary, const std::string& colName, int colType, const vtkVariant& val)
{
  std::string actualColumnName(colName);
  // We need to find a unique column name as close to colName that isn't taken.
  vtkAbstractArray* arr;
  int counter = 0;
  while ((arr = statSummary->GetColumnByName(actualColumnName.c_str())) != nullptr)
  {
    std::ostringstream os;
    os << colName << "_" << ++counter;
    actualColumnName = os.str();
  }
  arr = vtkAbstractArray::CreateArray(colType);
  arr->SetName(actualColumnName.c_str());
  arr->SetNumberOfTuples(1);
  arr->SetVariantValue(0, val);
  statSummary->AddColumn(arr);
  arr->FastDelete();
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkDataObject> vtkExtractDataArraysOverTime::vtkInternal::Summarize(
  vtkDataObject* input)
{
  assert(input != nullptr);

  const int attributeType = this->Self->GetFieldAssociation();
  vtkFieldData* inFD = input->GetAttributesAsFieldData(attributeType);
  assert(inFD != nullptr);

  const vtkIdType numIDs = inFD->GetNumberOfTuples();
  if (numIDs <= 0)
  {
    return nullptr;
  }

  // Make a vtkTable containing all fields plus possibly point coordinates.
  // We'll pass the table, after splitting multi-component arrays, to
  // vtkDescriptiveStatistics to get information about all the selected data at
  // this timestep.
  vtkNew<vtkTable> statInput;   // Input table created from input's attributes
  vtkNew<vtkTable> statSummary; // Reformatted statistics filter output
  vtkNew<vtkSplitColumnComponents> splitColumns;
  auto descrStats = this->Self->NewDescriptiveStatistics();
  auto orderStats = this->Self->NewOrderStatistics();
  descrStats->SetLearnOption(1);
  descrStats->SetDeriveOption(1);
  descrStats->SetAssessOption(0);
  orderStats->SetLearnOption(1);
  orderStats->SetDeriveOption(1);
  orderStats->SetAssessOption(0);

  vtkDataSetAttributes* statInDSA = statInput->GetRowData();
  statInDSA->ShallowCopy(inFD);
  // Add point coordinates to selected data if we are tracking point-data.
  if (attributeType == vtkDataObject::POINT)
  {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(input);
    vtkNew<vtkDoubleArray> pX[3];
    for (int comp = 0; comp < 3; ++comp)
    {
      pX[comp]->SetNumberOfComponents(1);
      pX[comp]->SetNumberOfTuples(numIDs);
    }
    for (vtkIdType cc = 0; cc < numIDs; ++cc)
    {
      double* coords = ds->GetPoint(cc);
      for (int comp = 0; comp < 3; ++comp)
      {
        pX[comp]->SetValue(cc, coords[comp]);
      }
    }
    vtkExtractArraysAssignUniqueCoordNames(statInDSA, pX[0], pX[1], pX[2]);
  }
  splitColumns->SetInputDataObject(0, statInput);
  splitColumns->SetCalculateMagnitudes(1);
  splitColumns->Update();
  vtkTable* splits = splitColumns->GetOutput();
  descrStats->SetInputConnection(splitColumns->GetOutputPort());
  orderStats->SetInputConnection(splitColumns->GetOutputPort());
  // Add a column holding the number of points/cells/rows
  // in the data at this timestep.
  vtkExtractArraysAddColumnValue(statSummary, "N", VTK_DOUBLE, numIDs);
  // Compute statistics 1 column at a time to save space (esp. for order stats)
  for (int i = 0; i < splits->GetNumberOfColumns(); ++i)
  {
    vtkAbstractArray* col = splits->GetColumn(i);
    int cType = col->GetDataType();
    const char* cname = col->GetName();
    orderStats->ResetRequests();
    orderStats->AddColumn(cname);
    orderStats->Update();
    vtkMultiBlockDataSet* order = vtkMultiBlockDataSet::SafeDownCast(
      orderStats->GetOutputDataObject(vtkStatisticsAlgorithm::OUTPUT_MODEL));
    if (order && order->GetNumberOfBlocks() >= 3)
    {
      vtkTable* model = vtkTable::SafeDownCast(order->GetBlock(2));
      std::ostringstream minName;
      std::ostringstream medName;
      std::ostringstream maxName;
      std::ostringstream q1Name;
      std::ostringstream q3Name;
      minName << "min(" << cname << ")";
      q1Name << "q1(" << cname << ")";
      medName << "med(" << cname << ")";
      q3Name << "q3(" << cname << ")";
      maxName << "max(" << cname << ")";
      vtkExtractArraysAddColumnValue(statSummary, minName.str(), cType, model->GetValue(0, 1));
      vtkExtractArraysAddColumnValue(statSummary, q1Name.str(), cType, model->GetValue(1, 1));
      vtkExtractArraysAddColumnValue(statSummary, medName.str(), cType, model->GetValue(2, 1));
      vtkExtractArraysAddColumnValue(statSummary, q3Name.str(), cType, model->GetValue(3, 1));
      vtkExtractArraysAddColumnValue(statSummary, maxName.str(), cType, model->GetValue(4, 1));
    }
    if (vtkArrayDownCast<vtkDataArray>(col))
    {
      descrStats->ResetRequests();
      descrStats->AddColumn(cname);
      descrStats->Update();
      vtkMultiBlockDataSet* descr = vtkMultiBlockDataSet::SafeDownCast(
        descrStats->GetOutputDataObject(vtkStatisticsAlgorithm::OUTPUT_MODEL));
      if (descr && descr->GetNumberOfBlocks() >= 2)
      { // block 0: raw model; block 1: derived model
        vtkTable* rawModel = vtkTable::SafeDownCast(descr->GetBlock(0));
        vtkTable* drvModel = vtkTable::SafeDownCast(descr->GetBlock(1));
        std::ostringstream avgName;
        std::ostringstream stdName;
        avgName << "avg(" << cname << ")";
        stdName << "std(" << cname << ")";
        vtkExtractArraysAddColumnValue(
          statSummary, avgName.str(), VTK_DOUBLE, rawModel->GetValueByName(0, "Mean"));
        vtkExtractArraysAddColumnValue(statSummary, stdName.str(), VTK_DOUBLE,
          drvModel->GetValueByName(0, "Standard Deviation"));
      }
    }
  }

  vtkDataSetAttributes* statOutDSA = statSummary->GetRowData();
  auto table = vtkSmartPointer<vtkTable>::New();
  table->SetRowData(statOutDSA);
  return table;
}

//----------------------------------------------------------------------------
void vtkExtractDataArraysOverTime::vtkInternal::AddTimeStepInternal(
  unsigned int composite_index, int ts_index, double vtkNotUsed(time), vtkDataObject* input)
{
  int attributeType = this->Self->GetFieldAssociation();
  const bool statsOnly = this->Self->GetReportStatisticsOnly();

  vtkSmartPointer<vtkDataObject> data = input;
  if (statsOnly)
  {
    // instead of saving raw-data, we're going to track the summary.
    data = this->Summarize(input);
    attributeType = vtkDataObject::ROW;
  }

  if (!data)
  {
    return;
  }

  vtkDataSetAttributes* inDSA = data->GetAttributes(attributeType);
  const vtkIdType numIDs = inDSA->GetNumberOfTuples();
  if (numIDs <= 0)
  {
    return;
  }

  vtkIdTypeArray* indexArray = nullptr;
  if (!statsOnly)
  {
    if (this->Self->GetUseGlobalIDs())
    {
      indexArray = vtkIdTypeArray::SafeDownCast(inDSA->GetGlobalIds());
    }
    else
    {
      // when not reporting stats, user can specify which array to use to index
      // elements.
      int association;
      indexArray =
        vtkIdTypeArray::SafeDownCast(this->Self->GetInputArrayToProcess(0, data, association));
      if (indexArray != nullptr && association != attributeType)
      {
        indexArray = nullptr;
      }
    }
  }

  const bool is_gid = (indexArray != nullptr && inDSA->GetGlobalIds() == indexArray);
  if (is_gid)
  {
    // if using global ids, then they are expected to be unique across
    // blocks. By discarding the composite-index, we can easily track
    // elements moving between blocks.
    composite_index = 0;
  }

  vtkDataSet* dsData = vtkDataSet::SafeDownCast(data);
  for (vtkIdType cc = 0; cc < numIDs; ++cc)
  {
    const vtkIdType curid = indexArray ? indexArray->GetTypedComponent(cc, 0) : cc;
    const vtkKey key(composite_index, curid);

    // This will allocate a new vtkTable is none is present
    vtkValue* value = this->GetOutput(key, inDSA, is_gid);
    vtkTable* output = value->Output;
    output->GetRowData()->CopyData(inDSA, cc, ts_index);

    // Mark the entry valid.
    value->ValidMaskArray->SetTypedComponent(ts_index, 0, 1);

    // Record the point coordinate if we are tracking a point.
    if (value->PointCoordinatesArray && dsData)
    {
      double coords[3];
      dsData->GetPoint(cc, coords);
      value->PointCoordinatesArray->SetTypedTuple(ts_index, coords);
    }
  }
}

//----------------------------------------------------------------------------
vtkExtractDataArraysOverTime::vtkInternal::vtkValue*
vtkExtractDataArraysOverTime::vtkInternal::GetOutput(
  const vtkKey& key, vtkDataSetAttributes* inDSA, bool using_gid)
{
  MapType::iterator iter = this->OutputGrids.find(key);
  if (iter == this->OutputGrids.end())
  {
    vtkValue value;
    vtkTable* output = vtkTable::New();
    value.Output.TakeReference(output);

    vtkDataSetAttributes* rowData = output->GetRowData();
    rowData->CopyAllocate(inDSA, this->NumberOfTimeSteps);
    // since CopyAllocate only allocates memory, but doesn't change the number
    // of tuples in each of the arrays, we need to do this explicitly.
    // see (paraview/paraview#18090).
    rowData->SetNumberOfTuples(this->NumberOfTimeSteps);

    // Add an array to hold the time at each step
    vtkDoubleArray* timeArray = this->TimeArray;
    if (inDSA && inDSA->GetArray("Time"))
    {
      timeArray->SetName("TimeData");
    }
    else
    {
      timeArray->SetName("Time");
    }

    if (this->Self->GetFieldAssociation() == vtkDataObject::POINT &&
      this->Self->GetReportStatisticsOnly() == false)
    {
      // These are the point coordinates of the original data
      vtkDoubleArray* coordsArray = vtkDoubleArray::New();
      coordsArray->SetNumberOfComponents(3);
      coordsArray->SetNumberOfTuples(this->NumberOfTimeSteps);
      if (inDSA && inDSA->GetArray("Point Coordinates"))
      {
        coordsArray->SetName("Points");
      }
      else
      {
        coordsArray->SetName("Point Coordinates");
      }
      std::fill_n(coordsArray->WritePointer(0, 3 * this->NumberOfTimeSteps),
        3 * this->NumberOfTimeSteps, 0.0);
      value.PointCoordinatesArray.TakeReference(coordsArray);
    }

    // This array is used to make particular samples as invalid.
    // This happens when we are looking at a location which is not contained
    // by a cell or at a cell or point id that is destroyed.
    // It is used in the parallel subclass as well.
    vtkCharArray* validPts = vtkCharArray::New();
    validPts->SetName("vtkValidPointMask");
    validPts->SetNumberOfComponents(1);
    validPts->SetNumberOfTuples(this->NumberOfTimeSteps);
    std::fill_n(validPts->WritePointer(0, this->NumberOfTimeSteps), this->NumberOfTimeSteps,
      static_cast<char>(0));
    value.ValidMaskArray.TakeReference(validPts);
    value.UsingGlobalIDs = using_gid;
    iter = this->OutputGrids.insert(MapType::value_type(key, value)).first;
  }
  else
  {
    if (iter->second.UsingGlobalIDs != using_gid)
    {
      // global id indication is mismatched over time. Should that ever happen?
      // Not sure.
    }
  }

  return &iter->second;
}

//****************************************************************************
vtkStandardNewMacro(vtkExtractDataArraysOverTime);
//----------------------------------------------------------------------------
vtkExtractDataArraysOverTime::vtkExtractDataArraysOverTime()
  : CurrentTimeIndex(0)
  , NumberOfTimeSteps(0)
  , FieldAssociation(vtkDataObject::POINT)
  , ReportStatisticsOnly(false)
  , UseGlobalIDs(true)
  , Error(vtkExtractDataArraysOverTime::NoError)
  , Internal(nullptr)
{
  this->SetNumberOfInputPorts(1);
  // set to something that we know will never select that array (as
  // we want the user to explicitly set it).
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_NONE, "-invalid-array-");
}

//----------------------------------------------------------------------------
vtkExtractDataArraysOverTime::~vtkExtractDataArraysOverTime()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkExtractDataArraysOverTime::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldAssociation: " << this->FieldAssociation << endl;
  os << indent << "ReportStatisticsOnly: " << this->ReportStatisticsOnly << endl;
  os << indent << "UseGlobalIDs: " << this->UseGlobalIDs << endl;
  os << indent << "NumberOfTimeSteps: " << this->NumberOfTimeSteps << endl;
}

//----------------------------------------------------------------------------
int vtkExtractDataArraysOverTime::FillInputPortInformation(int, vtkInformation* info)
{
  // We can handle composite datasets.
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractDataArraysOverTime::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    this->NumberOfTimeSteps = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }
  else
  {
    this->NumberOfTimeSteps = 0;
  }

  // The output of this filter does not contain a specific time, rather
  // it contains a collection of time steps. Also, this filter does not
  // respond to time requests. Therefore, we remove all time information
  // from the output.
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractDataArraysOverTime::RequestUpdateExtent(vtkInformation*,
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  // get the requested update extent
  const double* inTimes = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (inTimes && this->CurrentTimeIndex >= 0)
  {
    assert(inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) > this->CurrentTimeIndex);
    double timeReq = inTimes[this->CurrentTimeIndex];
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeReq);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractDataArraysOverTime::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (this->NumberOfTimeSteps <= 0)
  {
    vtkErrorMacro("No time steps in input data!");
    return 0;
  }

  if (this->FieldAssociation == vtkDataObject::FIELD ||
    this->FieldAssociation == vtkDataObject::POINT_THEN_CELL || this->FieldAssociation < 0 ||
    this->FieldAssociation >= vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES)
  {
    vtkErrorMacro("Unsupported FieldAssociation '" << this->FieldAssociation << "'.");
    return 0;
  }

  // is this the first request?
  if (this->Internal == nullptr)
  {
    this->Internal = new vtkInternal(this->NumberOfTimeSteps, this);
    this->Error = vtkExtractDataArraysOverTime::NoError;
    this->CurrentTimeIndex = 0;

    // Tell the pipeline to start looping.
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
  }

  assert(this->Internal);

  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
  const double time_step = input->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
  this->Internal->AddTimeStep(this->CurrentTimeIndex, time_step, input);
  this->UpdateProgress(static_cast<double>(this->CurrentTimeIndex) / this->NumberOfTimeSteps);

  // increment the time index
  this->CurrentTimeIndex++;
  if (this->CurrentTimeIndex == this->NumberOfTimeSteps)
  {
    this->PostExecute(request, inputVector, outputVector);
    delete this->Internal;
    this->Internal = nullptr;
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractDataArraysOverTime::PostExecute(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Tell the pipeline to stop looping.
  request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
  this->CurrentTimeIndex = 0;
  this->Internal->CollectTimesteps(
    vtkDataObject::GetData(inputVector[0], 0), vtkMultiBlockDataSet::GetData(outputVector, 0));
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDescriptiveStatistics> vtkExtractDataArraysOverTime::NewDescriptiveStatistics()
{
  return vtkSmartPointer<vtkDescriptiveStatistics>::New();
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkOrderStatistics> vtkExtractDataArraysOverTime::NewOrderStatistics()
{
  return vtkSmartPointer<vtkOrderStatistics>::New();
}
