/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractArraysOverTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractArraysOverTime.h"

#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataSetAttributes.h"
#include "vtkDataSet.h"
#include "vtkDescriptiveStatistics.h"
#include "vtkDoubleArray.h"
#include "vtkExtractSelection.h"
#include "vtkGenericCell.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOrderStatistics.h"
#include "vtkPointData.h"
#include "vtkTable.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkSplitColumnComponents.h"
#include "vtkStdString.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"

#include <cassert>
#include <map>
#include <string>
#include <sstream>
#include <vector>

class vtkExtractArraysOverTime::vtkInternal
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

public: // vtkValue is made public due to a bug in VS 6.0
  class vtkValue
  {
  public:
    std::string Label;
    vtkSmartPointer<vtkTable> Output;
    vtkSmartPointer<vtkUnsignedCharArray> ValidMaskArray;
    vtkSmartPointer<vtkDoubleArray> PointCoordinatesArray;
  };
private:
  typedef std::map<vtkKey, vtkValue> MapType;
  MapType OutputGrids;
  int NumberOfTimeSteps;
  int CurrentTimeIndex;
  int FieldType;
  int ContentType;
  int ReportStatisticsOnly;

  void AddTimeStepInternal(unsigned int cid, double time, vtkDataObject* data);
  void AddTimeStepInternalForLocations(unsigned int composite_index,
    double time, vtkDataSet* input);
  void AddTimeStepInternalForQuery(unsigned int composite_index,
    double time, vtkDataObject* input);
  vtkValue* GetOutput(const vtkKey& key, vtkDataSetAttributes* inDSA);

  void RemoveInvalidPoints(vtkUnsignedCharArray* validArray,
    vtkDataSetAttributes* pd)
  {
    vtkIdType numIDs = validArray->GetNumberOfTuples();
    for (vtkIdType cc=0; cc < numIDs; cc++)
    {
      if (validArray->GetValue(cc) != 1)
      {
        //an invalid sample, set all the data values to 0.0
        vtkIdType narrays = pd->GetNumberOfArrays();
        for (vtkIdType a = 0; a < narrays; a++)
        {
          vtkDataArray *da = pd->GetArray(a);
          if (da != validArray && da != this->TimeArray.GetPointer())
          {
            for (vtkIdType j = 0; j < da->GetNumberOfComponents(); j++)
            {
              da->SetComponent(cc, j, 0.0);
            }
          }
        }
      }

    }
  }

  // We use the same time array for all extracted time lines, since that doesn't
  // change.
  vtkSmartPointer<vtkDoubleArray> TimeArray;
public:
  // List of ids selected for fast path.
  vtkInternal()
  {
    this->NumberOfTimeSteps = 0;
    this->FieldType = 0;
    this->CurrentTimeIndex = 0;
    this->ContentType = -1;
    this->ReportStatisticsOnly = 0;
  }

  // Description:
  // Intializes the data structure.
  void Initialize(
    int numTimeSteps, int contentType, int fieldType, int statsOnly)
  {
    this->CurrentTimeIndex = 0;
    this->NumberOfTimeSteps = numTimeSteps;
    this->FieldType = fieldType;
    this->ContentType = contentType;
    this->OutputGrids.clear();

    this->TimeArray = vtkSmartPointer<vtkDoubleArray>::New();
    this->TimeArray->SetNumberOfTuples(this->NumberOfTimeSteps);
    this->TimeArray->FillComponent(0, 0);

    this->ReportStatisticsOnly = statsOnly;
  }

  // Description:
  // Add the output of the extract selection filter.
  void AddTimeStep(double time, vtkDataObject* data);

  // Description:
  // Collect the gathered timesteps into the output.
  void CollectTimesteps(vtkMultiBlockDataSet* output)
  {
    output->Initialize();
    MapType::iterator iter;
    unsigned int cc=0;
    for (iter = this->OutputGrids.begin();
      iter != this->OutputGrids.end(); ++iter)
    {
      if (iter->second.Output.GetPointer())
      {
        vtkValue& value = iter->second;

        // TODO; To add information about where which cell/pt this grid came
        // from.

        // Remove vtkOriginalCellIds or vtkOriginalPointIds arrays which were
        // added by vtkExtractSelection.
        value.Output->GetRowData()->RemoveArray("vtkOriginalCellIds");
        value.Output->GetRowData()->RemoveArray("vtkOriginalPointIds");

        value.Output->GetRowData()->RemoveArray(
          value.ValidMaskArray->GetName());
        value.Output->GetRowData()->AddArray(value.ValidMaskArray);

        value.Output->GetRowData()->RemoveArray(
          this->TimeArray->GetName());
        value.Output->GetRowData()->AddArray(this->TimeArray);

        // Only add point coordinates when not reporting selection statistics.
        // We never report statistics for LOCATION queries, but do for other
        // cases where FieldType == POINT, and always report statistics for
        // QUERY selections.
        if (value.PointCoordinatesArray &&
          (this->FieldType != vtkSelectionNode::POINT ||
           !(this->ReportStatisticsOnly ||
             this->ContentType == vtkSelectionNode::QUERY)))
        {
          value.Output->GetRowData()->RemoveArray(
            value.PointCoordinatesArray->GetName());
          value.Output->GetRowData()->AddArray(value.PointCoordinatesArray);
        }

        this->RemoveInvalidPoints(value.ValidMaskArray,
          value.Output->GetRowData());
        output->SetBlock(cc, value.Output.GetPointer());
        output->GetMetaData(cc)->Set(vtkCompositeDataSet::NAME(),
          value.Label.c_str());
        cc++;
      }
    }

    this->OutputGrids.clear();
  }
};

//----------------------------------------------------------------------------
void vtkExtractArraysOverTime::vtkInternal::AddTimeStep(
  double time, vtkDataObject* data)
{
  this->TimeArray->SetTuple1(this->CurrentTimeIndex, time);

  if (data && (data->IsA("vtkDataSet") || data->IsA("vtkTable")))
  {
    this->AddTimeStepInternal(0, time, data);
  }
  else if (data && data->IsA("vtkCompositeDataSet"))
  {
    vtkCompositeDataSet* cd = reinterpret_cast<vtkCompositeDataSet*>(data);
    vtkCompositeDataIterator* iter = cd->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (ds)
      {
        this->AddTimeStepInternal(iter->GetCurrentFlatIndex(), time, ds);
      }
      else if (
        vtkTable* table = vtkTable::SafeDownCast(iter->GetCurrentDataObject()))
      {
        this->AddTimeStepInternal(iter->GetCurrentFlatIndex(), time, table);
      }
    }
    iter->Delete();
  }

  this->CurrentTimeIndex++;
}

//----------------------------------------------------------------------------
void vtkExtractArraysOverTime::vtkInternal::AddTimeStepInternalForLocations(
  unsigned int vtkNotUsed(composite_index), double vtkNotUsed(time), vtkDataSet* input)
{
  if (!input)
  {
    vtkGenericWarningMacro("Ignoring since input is not a vtkDataset.");
    return;
  }

  vtkDataSetAttributes* inDSA = input->GetPointData();
  vtkCharArray* validMask = vtkArrayDownCast<vtkCharArray>(
    inDSA->GetArray("vtkValidPointMask"));

  if (!validMask)
  {
    vtkGenericWarningMacro("Missing \"vtkValidPointMask\" in extracted dataset.");
    return;
  }

  vtkIdType numIDs = validMask->GetNumberOfTuples();
  if (numIDs <= 0)
  {
    return;
  }

  for (vtkIdType cc=0; cc < numIDs; cc++)
  {
    char valid = validMask->GetValue(cc);
    if (valid == 0)
    {
      continue;
    }

    // When probing locations, each timeline corresponds to each of the probe
    // locations. Hence, the key is just the index of the probe location and the
    // not the selected cell/point id.
    vtkKey key(0, cc);

    // This will allocate a new vtkTable is none is present
    vtkValue* value = this->GetOutput(key, inDSA);

    vtkTable* output = value->Output;
    output->GetRowData()->CopyData(inDSA, cc, this->CurrentTimeIndex);

    // Mark the entry valid.
    value->ValidMaskArray->SetValue(this->CurrentTimeIndex, 1);

    // Record the point coordinate if we are tracking a point.
    double *point = input->GetPoint(cc);
    value->PointCoordinatesArray->SetTuple(this->CurrentTimeIndex, point);

    if (value->Label.empty())
    {
      std::ostringstream stream;
      stream << "(" << point[0] << ", " << point[1] << ", " << point[2] << ")";
      value->Label = stream.str();
    }
  }
}

//----------------------------------------------------------------------------
static void vtkExtractArraysAssignUniqueCoordNames(
  vtkDataSetAttributes* statInDSA,
  vtkDataArray* px, vtkDataArray* py, vtkDataArray* pz)
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
  while (
    (arrX = statInDSA->GetArray(actualNames[0].c_str())) != NULL &&
    (arrY = statInDSA->GetArray(actualNames[1].c_str())) != NULL &&
    (arrZ = statInDSA->GetArray(actualNames[2].c_str())) != NULL)
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
  vtkTable* statSummary, const std::string& colName, int colType,
  const vtkVariant& val)
{
  std::string actualColumnName(colName);
  // We need to find a unique column name as close to colName that isn't taken.
  vtkAbstractArray* arr;
  int counter = 0;
  while ((arr = statSummary->GetColumnByName(actualColumnName.c_str())) != NULL)
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
void vtkExtractArraysOverTime::vtkInternal::AddTimeStepInternalForQuery(
  unsigned int composite_index, double vtkNotUsed(time), vtkDataObject* input)
{
  vtkFieldData* inFD = 0;
  if (this->FieldType == vtkSelectionNode::CELL)
  {
    inFD = vtkDataSet::SafeDownCast(input)->GetCellData();
  }
  else if (this->FieldType == vtkSelectionNode::POINT)
  {
    inFD = vtkDataSet::SafeDownCast(input)->GetPointData();
  }
  else if (this->FieldType == vtkSelectionNode::ROW)
  {
    inFD = vtkTable::SafeDownCast(input)->GetRowData();
  }
  else if (this->FieldType == vtkSelectionNode::FIELD)
  {
    inFD = input->GetFieldData();
  }

  if (!inFD)
  { // We don't handle graph selections yet
    vtkGenericWarningMacro(
      "Ignoring unsupported field type " << this->FieldType << ".");
    return;
  }

  vtkIdType numIDs = inFD->GetNumberOfTuples();
  if (numIDs <= 0)
  {
    return;
  }

  // Make a vtkTable containing all fields plus possibly point coordinates.
  // We'll pass the table, after splitting multi-component arrays, to
  // vtkDescriptiveStatistics to get information about all the selected data at
  // this timestep.
  vtkNew<vtkTable> statInput; // Input table created from selection's attributes
  vtkNew<vtkTable> statSummary; // Reformatted statistics filter output
  vtkNew<vtkSplitColumnComponents> splitColumns;
  vtkNew<vtkDescriptiveStatistics> descrStats;
  vtkNew<vtkOrderStatistics> orderStats;
  descrStats->SetLearnOption(1);
  descrStats->SetDeriveOption(1);
  descrStats->SetAssessOption(0);
  orderStats->SetLearnOption(1);
  orderStats->SetDeriveOption(1);
  orderStats->SetAssessOption(0);

  vtkDataSetAttributes* statInDSA = statInput->GetRowData();
  statInDSA->ShallowCopy(inFD);
  // Add point coordinates to selected data if we are tracking point-data.
  if (this->FieldType == vtkSelectionNode::POINT)
  {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(input);
    vtkNew<vtkDoubleArray> pX[3];
    int comp;
    for (comp = 0; comp < 3; ++comp)
    {
      pX[comp]->SetNumberOfComponents(1);
      pX[comp]->SetNumberOfTuples(numIDs);
    }
    for (vtkIdType cc = 0; cc < numIDs; ++cc)
    {
      double* coords = ds->GetPoint(cc);
      for (comp = 0; comp < 3; ++comp)
      {
        pX[comp]->SetValue(cc, coords[comp]);
      }
    }
    vtkExtractArraysAssignUniqueCoordNames(
      statInDSA, pX[0].GetPointer(), pX[1].GetPointer(), pX[2].GetPointer());
  }
  splitColumns->SetInputDataObject(0, statInput.GetPointer());
  splitColumns->SetCalculateMagnitudes(1);
  splitColumns->Update();
  vtkTable* splits = splitColumns->GetOutput();
  descrStats->SetInputConnection(splitColumns->GetOutputPort());
  orderStats->SetInputConnection(splitColumns->GetOutputPort());
  // Add a column holding the number of points/cells/rows
  // in the selection at this timestep.
  vtkExtractArraysAddColumnValue(
    statSummary.GetPointer(), "N", VTK_DOUBLE, numIDs);
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
      q1Name  <<  "q1(" << cname << ")";
      medName << "med(" << cname << ")";
      q3Name  <<  "q3(" << cname << ")";
      maxName << "max(" << cname << ")";
      vtkExtractArraysAddColumnValue(
        statSummary.GetPointer(), minName.str(), cType, model->GetValue(0, 1));
      vtkExtractArraysAddColumnValue(
        statSummary.GetPointer(),  q1Name.str(), cType, model->GetValue(1, 1));
      vtkExtractArraysAddColumnValue(
        statSummary.GetPointer(), medName.str(), cType, model->GetValue(2, 1));
      vtkExtractArraysAddColumnValue(
        statSummary.GetPointer(),  q3Name.str(), cType, model->GetValue(3, 1));
      vtkExtractArraysAddColumnValue(
        statSummary.GetPointer(), maxName.str(), cType, model->GetValue(4, 1));
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
          statSummary.GetPointer(), avgName.str(), VTK_DOUBLE,
          rawModel->GetValueByName(0, "Mean"));
        vtkExtractArraysAddColumnValue(
          statSummary.GetPointer(), stdName.str(), VTK_DOUBLE,
          drvModel->GetValueByName(0, "Standard Deviation"));
      }
    }
  }

  vtkDataSetAttributes* statOutDSA = statSummary->GetRowData();

  // This will allocate a new vtkTable is none is present for key
  vtkKey key(composite_index, 0);
  vtkValue* value= this->GetOutput(key, statOutDSA);
  vtkTable* output = value->Output;
  output->GetRowData()->CopyData(statOutDSA, 0, this->CurrentTimeIndex);

  // Mark the entry valid.
  value->ValidMaskArray->SetValue(this->CurrentTimeIndex, 1);

  // Determine the label to use for this block if none exists.
  if (value->Label.empty())
  {
    std::ostringstream stream;
    if (value->Label.empty())
    {
      if (composite_index != 0)
      {
        stream << "Block: " << composite_index << " ; ";
      }
      switch (this->FieldType)
      {
      case vtkSelectionNode::CELL:
        stream << "Cell ";
        break;

      case vtkSelectionNode::POINT:
        stream << "Point ";
        break;

      case vtkSelectionNode::ROW:
        stream << "Row " ;
        break;
      }
      stream << "Statistics";
      value->Label = stream.str();
    }
  }
}

//----------------------------------------------------------------------------
void vtkExtractArraysOverTime::vtkInternal::AddTimeStepInternal(
  unsigned int composite_index, double time, vtkDataObject* input)
{
  if (this->ContentType == vtkSelectionNode::LOCATIONS)
  {
    this->AddTimeStepInternalForLocations(composite_index, time,
      vtkDataSet::SafeDownCast(input));
    return;
  }
  else if (
    this->ContentType == vtkSelectionNode::QUERY ||
    this->ReportStatisticsOnly)
  {
    this->AddTimeStepInternalForQuery(composite_index, time, input);
    return;
  }

  vtkDataSetAttributes* inDSA = 0;
  const char* idarrayname = 0;
  if (this->FieldType == vtkSelectionNode::CELL)
  {
    inDSA = vtkDataSet::SafeDownCast(input)->GetCellData();
    idarrayname = "vtkOriginalCellIds";
  }
  else if (this->FieldType == vtkSelectionNode::POINT)
  {
    inDSA = vtkDataSet::SafeDownCast(input)->GetPointData();
    idarrayname = "vtkOriginalPointIds";
  }
  else if (this->FieldType == vtkSelectionNode::ROW)
  {
    inDSA = vtkTable::SafeDownCast(input)->GetRowData();
    idarrayname = "vtkOriginalRowIds";
  }
  else
  {
    vtkGenericWarningMacro("Ignoring since unsupported field type.");
    return;
  }

  vtkIdTypeArray* idsArray =
    vtkArrayDownCast<vtkIdTypeArray>(inDSA->GetArray(idarrayname));

  if (this->ContentType == vtkSelectionNode::GLOBALIDS)
  {
    idsArray = vtkArrayDownCast<vtkIdTypeArray>(inDSA->GetGlobalIds());
  }

  if (!idsArray)
  {
    vtkGenericWarningMacro("Missing \"" << idarrayname << "\" in extracted dataset.");
    return;
  }

  vtkIdType numIDs = idsArray->GetNumberOfTuples();
  if (numIDs <= 0)
  {
    return;
  }

  for (vtkIdType cc=0; cc < numIDs; cc++)
  {
    vtkIdType curid = idsArray->GetValue(cc);
    vtkKey key(composite_index, curid);

    // This will allocate a new vtkTable is none is present
    vtkValue* value= this->GetOutput(key, inDSA);
    vtkTable* output = value->Output;
    output->GetRowData()->CopyData(inDSA, cc, this->CurrentTimeIndex);

    // Mark the entry valid.
    value->ValidMaskArray->SetValue(this->CurrentTimeIndex, 1);

    // Record the point coordinate if we are tracking a point.
    if (value->PointCoordinatesArray)
    {
      double *point = vtkDataSet::SafeDownCast(input)->GetPoint(cc);
      value->PointCoordinatesArray->SetTuple(this->CurrentTimeIndex, point);
    }

    // Determine the label to use for this block if none has been already
    // assigned.
    if (value->Label.empty())
    {
      std::ostringstream stream;
      if (this->ContentType == vtkSelectionNode::GLOBALIDS)
      {
        vtkIdTypeArray* gidsArray = vtkArrayDownCast<vtkIdTypeArray>(
          inDSA->GetGlobalIds());
        if (gidsArray)
        {
          stream << "GlobalID: " << gidsArray->GetValue(cc);
          value->Label = stream.str();
        }
      }
      if (value->Label.empty())
      {
        if (composite_index != 0)
        {
          stream << "Block: " << composite_index << " ; ";
        }
        switch (this->FieldType)
        {
        case vtkSelectionNode::CELL:
          stream << "Cell : ";
          break;

        case vtkSelectionNode::POINT:
          stream << "Point : ";
          break;

        case vtkSelectionNode::ROW:
          stream << "Row: " ;
          break;
        }
        stream << curid;
        value->Label = stream.str();
      }
    }
  }
}

//----------------------------------------------------------------------------
vtkExtractArraysOverTime::vtkInternal::vtkValue*
vtkExtractArraysOverTime::vtkInternal::GetOutput(
  const vtkKey& key, vtkDataSetAttributes* inDSA)
{
  MapType::iterator iter = this->OutputGrids.find(key);

  if (iter == this->OutputGrids.end())
  {
    vtkValue value;
    vtkTable *output = vtkTable::New();
    value.Output.TakeReference(output);

    vtkDataSetAttributes *rowData = output->GetRowData();
    if (this->ContentType == vtkSelectionNode::LOCATIONS)
    {
      rowData->InterpolateAllocate(inDSA, this->NumberOfTimeSteps);
    }
    else
    {
      rowData->CopyAllocate(inDSA, this->NumberOfTimeSteps);
    }

    // Add an array to hold the time at each step
    vtkDoubleArray *timeArray = this->TimeArray;
    if (inDSA && inDSA->GetArray("Time"))
    {
      timeArray->SetName("TimeData");
    }
    else
    {
      timeArray->SetName("Time");
    }

    if (this->FieldType == vtkSelectionNode::POINT ||
      this->ContentType == vtkSelectionNode::LOCATIONS)
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
      if (this->ContentType == vtkSelectionNode::LOCATIONS)
      {
        coordsArray->SetName("Probe Coordinates");
      }
      coordsArray->FillComponent(0, 0.0);
      coordsArray->FillComponent(1, 0.0);
      coordsArray->FillComponent(2, 0.0);
      value.PointCoordinatesArray.TakeReference(coordsArray);
    }

    // This array is used to make particular samples as invalid.
    // This happens when we are looking at a location which is not contained
    // by a cell or at a cell or point id that is destroyed.
    // It is used in the parallel subclass as well.
    vtkUnsignedCharArray* validPts = vtkUnsignedCharArray::New();
    validPts->SetName("vtkValidPointMask");
    validPts->SetNumberOfComponents(1);
    validPts->SetNumberOfTuples(this->NumberOfTimeSteps);
    validPts->FillComponent(0, 0);
    value.ValidMaskArray.TakeReference(validPts);

    iter = this->OutputGrids.insert(MapType::value_type(key, value)).first;
  }

  return &iter->second;
}

//****************************************************************************
vtkStandardNewMacro(vtkExtractArraysOverTime);
vtkCxxSetObjectMacro(vtkExtractArraysOverTime, SelectionExtractor, vtkExtractSelection);
//----------------------------------------------------------------------------
vtkExtractArraysOverTime::vtkExtractArraysOverTime()
{
  this->NumberOfTimeSteps = 0;
  this->CurrentTimeIndex = 0;
  this->ReportStatisticsOnly = 0;

  this->SetNumberOfInputPorts(2);

  this->ContentType = -1;
  this->FieldType = vtkSelectionNode::CELL;

  this->Error = vtkExtractArraysOverTime::NoError;

  this->SelectionExtractor = NULL;

  this->Internal = new vtkInternal;

  this->IsExecuting = false;
}

//----------------------------------------------------------------------------
vtkExtractArraysOverTime::~vtkExtractArraysOverTime()
{
  delete this->Internal;
  this->SetSelectionExtractor(NULL);
}

//----------------------------------------------------------------------------
void vtkExtractArraysOverTime::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "NumberOfTimeSteps: " << this->NumberOfTimeSteps << endl;
  os << indent << "SelectionExtractor: " << this->SelectionExtractor << endl;
  os << indent << "ReportStatisticsOnly: " <<
    (this->ReportStatisticsOnly ? "ON" : "OFF") << endl;
}

//----------------------------------------------------------------------------
int vtkExtractArraysOverTime::FillInputPortInformation(
  int port, vtkInformation* info)
{
  if (port==0)
  {
    // We can handle composite datasets.
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  }
  else
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractArraysOverTime::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  if ( inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) )
  {
    this->NumberOfTimeSteps =
      inInfo->Length( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
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
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
  {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractArraysOverTime::RequestUpdateExtent(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  // vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo1 = inputVector[0]->GetInformationObject(0);

  // get the requested update extent
  double *inTimes = inInfo1->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (inTimes)
  {
    double timeReq= inTimes[this->CurrentTimeIndex];
    inInfo1->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeReq);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractArraysOverTime::RequestData(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (this->NumberOfTimeSteps == 0)
  {
    vtkErrorMacro("No time steps in input data!");
    return 0;
  }

  // get the output data object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // is this the first request
  if (!this->IsExecuting)
  {
    vtkInformation* inInfo2 = inputVector[1]->GetInformationObject(0);
    vtkSelection* selection = this->GetSelection(inInfo2);
    if (!selection)
    {
      return 1;
    }
    if (!this->DetermineSelectionType(selection))
    {
      return 0;
    }

    // Tell the pipeline to start looping.
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);

    this->Internal->Initialize(
      this->NumberOfTimeSteps, this->ContentType,
      this->FieldType, this->ReportStatisticsOnly);

    this->Error = vtkExtractArraysOverTime::NoError;

    this->IsExecuting = true;
  }

  // If we get here, there is no fast-path option available.
  this->ExecuteAtTimeStep(inputVector, outInfo);

  // increment the time index
  this->CurrentTimeIndex++;
  if (this->CurrentTimeIndex == this->NumberOfTimeSteps)
  {
    this->PostExecute(request, inputVector, outputVector);
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractArraysOverTime::PostExecute(
  vtkInformation* request,
  vtkInformationVector**,
  vtkInformationVector* outputVector)
{
  // Tell the pipeline to stop looping.
  request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
  this->CurrentTimeIndex = 0;
  this->IsExecuting = false;

//switch (this->Error)
//  {
//  case vtkExtractArraysOverTime::MoreThan1Indices:
//    vtkErrorMacro(<< "This filter can extract only 1 cell or "
//                  << " point at a time. Only the first index"
//                  << " was extracted");
//
//  }

  //Use the vtkValidPointMask array to zero any invalid samples.
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::GetData(outInfo);
  this->Internal->CollectTimesteps(output);
}

//----------------------------------------------------------------------------
void vtkExtractArraysOverTime::ExecuteAtTimeStep(
  vtkInformationVector** inputVector,
  vtkInformation* outInfo)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *selInfo = inputVector[1]->GetInformationObject(0);

  vtkDataObject* input = vtkDataObject::GetData(inInfo);
  vtkSelection* selInput = this->GetSelection(selInfo);

  vtkExtractSelection* filter = this->SelectionExtractor;
  if (!filter)
  {
    vtkNew<vtkExtractSelection> extractor;
    this->SetSelectionExtractor(extractor.GetPointer());
    filter = extractor.GetPointer();
  }
  filter->SetPreserveTopology(0);
  filter->SetUseProbeForLocations(1);
  filter->SetInputData(0, input);
  filter->SetInputData(1, selInput);

  vtkDebugMacro(<< "Preparing subfilter to extract from dataset");
  //pass all required information to the helper filter
  int piece = 0;
  int npieces = 1;
  int *uExtent=0;
  if (outInfo->Has(
        vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
  {
    piece = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    npieces = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  }

  if (outInfo->Has(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()))
  {
    uExtent = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  }

  filter->UpdatePiece(piece, npieces, 0, uExtent);

  vtkDataObject* output = filter->GetOutputDataObject(0)->NewInstance();
  output->ShallowCopy(filter->GetOutputDataObject(0));

  double time_step = input->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
  this->Internal->AddTimeStep(time_step, output);

  output->Delete();

  this->UpdateProgress(
    static_cast<double>(this->CurrentTimeIndex)/this->NumberOfTimeSteps);
}

//----------------------------------------------------------------------------
int vtkExtractArraysOverTime::DetermineSelectionType(vtkSelection* sel)
{
  int contentType = -1;
  int fieldType = -1;
  unsigned int numNodes = sel->GetNumberOfNodes();
  for (unsigned int cc = 0; cc < numNodes; cc++)
  {
    vtkSelectionNode* node = sel->GetNode(cc);
    if (node)
    {
      int nodeFieldType = node->GetFieldType();
      int nodeContentType = node->GetContentType();
      if ((fieldType != -1 && fieldType != nodeFieldType) ||
        (contentType != -1 && contentType != nodeContentType))
      {
        vtkErrorMacro("All vtkSelectionNode instances within a vtkSelection"
          " must have the same ContentType and FieldType.");
        return 0;
      }
      fieldType = nodeFieldType;
      contentType = nodeContentType;
    }
  }
  this->ContentType = contentType;
  this->FieldType = fieldType;
  return 1;
}
//----------------------------------------------------------------------------
vtkSelection* vtkExtractArraysOverTime::GetSelection(vtkInformation* info)
{
  return vtkSelection::GetData(info);
}
