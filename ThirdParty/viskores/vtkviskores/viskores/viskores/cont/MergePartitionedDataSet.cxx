//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleGroupVecVariable.h>
#include <viskores/cont/ArrayHandleView.h>
#include <viskores/cont/ConvertNumComponentsToOffsets.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Logging.h>
#include <viskores/cont/MergePartitionedDataSet.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/cont/UnknownCellSet.h>
#include <viskores/cont/internal/CastInvalidValue.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>


namespace
{

struct CopyWithOffsetWorklet : public viskores::worklet::WorkletMapField
{
  viskores::Id OffsetValue;
  VISKORES_CONT
  CopyWithOffsetWorklet(const viskores::Id offset)
    : OffsetValue(offset)
  {
  }
  typedef void ControlSignature(FieldIn, FieldInOut);
  typedef void ExecutionSignature(_1, _2);
  VISKORES_EXEC void operator()(const viskores::Id originalValue, viskores::Id& outputValue) const
  {
    outputValue = originalValue + this->OffsetValue;
  }
};

void CountPointsAndCells(const viskores::cont::PartitionedDataSet& partitionedDataSet,
                         viskores::Id& numPointsTotal,
                         viskores::Id& numCellsTotal)
{
  numPointsTotal = 0;
  numCellsTotal = 0;

  for (viskores::Id partitionId = 0; partitionId < partitionedDataSet.GetNumberOfPartitions();
       ++partitionId)
  {
    viskores::cont::DataSet partition = partitionedDataSet.GetPartition(partitionId);
    numPointsTotal += partition.GetNumberOfPoints();
    numCellsTotal += partition.GetNumberOfCells();
  }
}

struct PassCellShapesNumIndices : viskores::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn inputTopology, FieldOut shapes, FieldOut numIndices);
  using ExecutionSignature = void(CellShape, PointCount, _2, _3);

  template <typename CellShape>
  VISKORES_EXEC void operator()(const CellShape& inShape,
                                viskores::IdComponent inNumIndices,
                                viskores::UInt8& outShape,
                                viskores::IdComponent& outNumIndices) const
  {
    outShape = inShape.Id;
    outNumIndices = inNumIndices;
  }
};

void MergeShapes(const viskores::cont::PartitionedDataSet& partitionedDataSet,
                 viskores::Id numCellsTotal,
                 viskores::cont::ArrayHandle<viskores::UInt8>& shapes,
                 viskores::cont::ArrayHandle<viskores::IdComponent>& numIndices,
                 const viskores::Id firstNonEmptyPartitionId)
{
  viskores::cont::Invoker invoke;

  shapes.Allocate(numCellsTotal);
  numIndices.Allocate(numCellsTotal);

  viskores::Id cellStartIndex = 0;
  for (viskores::Id partitionId = firstNonEmptyPartitionId;
       partitionId < partitionedDataSet.GetNumberOfPartitions();
       ++partitionId)
  {
    if (partitionedDataSet.GetPartition(partitionId).GetNumberOfPoints() == 0)
    {
      continue;
    }
    viskores::cont::DataSet partition = partitionedDataSet.GetPartition(partitionId);
    viskores::Id numCellsPartition = partition.GetNumberOfCells();

    auto shapesView =
      viskores::cont::make_ArrayHandleView(shapes, cellStartIndex, numCellsPartition);
    auto numIndicesView =
      viskores::cont::make_ArrayHandleView(numIndices, cellStartIndex, numCellsPartition);

    invoke(PassCellShapesNumIndices{}, partition.GetCellSet(), shapesView, numIndicesView);

    cellStartIndex += numCellsPartition;
  }
  VISKORES_ASSERT(cellStartIndex == numCellsTotal);
}

struct PassCellIndices : viskores::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn inputTopology, FieldOut pointIndices);
  using ExecutionSignature = void(PointIndices, _2);

  viskores::Id IndexOffset;

  PassCellIndices(viskores::Id indexOffset)
    : IndexOffset(indexOffset)
  {
  }

  template <typename InPointIndexType, typename OutPointIndexType>
  VISKORES_EXEC void operator()(const InPointIndexType& inPoints,
                                OutPointIndexType& outPoints) const
  {
    viskores::IdComponent numPoints = inPoints.GetNumberOfComponents();
    VISKORES_ASSERT(numPoints == outPoints.GetNumberOfComponents());
    for (viskores::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      outPoints[pointIndex] = inPoints[pointIndex] + this->IndexOffset;
    }
  }
};

void MergeIndices(const viskores::cont::PartitionedDataSet& partitionedDataSet,
                  const viskores::cont::ArrayHandle<viskores::Id>& offsets,
                  viskores::Id numIndicesTotal,
                  viskores::cont::ArrayHandle<viskores::Id>& indices,
                  const viskores::Id firstNonEmptyPartitionId)
{
  viskores::cont::Invoker invoke;

  indices.Allocate(numIndicesTotal);

  viskores::Id pointStartIndex = 0;
  viskores::Id cellStartIndex = 0;
  for (viskores::Id partitionId = firstNonEmptyPartitionId;
       partitionId < partitionedDataSet.GetNumberOfPartitions();
       ++partitionId)
  {
    if (partitionedDataSet.GetPartition(partitionId).GetNumberOfPoints() == 0)
    {
      continue;
    }
    viskores::cont::DataSet partition = partitionedDataSet.GetPartition(partitionId);
    viskores::Id numCellsPartition = partition.GetNumberOfCells();

    auto offsetsView =
      viskores::cont::make_ArrayHandleView(offsets, cellStartIndex, numCellsPartition + 1);
    auto indicesGroupView = viskores::cont::make_ArrayHandleGroupVecVariable(indices, offsetsView);

    invoke(PassCellIndices{ pointStartIndex }, partition.GetCellSet(), indicesGroupView);

    pointStartIndex += partition.GetNumberOfPoints();
    cellStartIndex += numCellsPartition;
  }
  VISKORES_ASSERT(cellStartIndex == (offsets.GetNumberOfValues() - 1));
}

viskores::cont::CellSetSingleType<> MergeCellSetsSingleType(
  const viskores::cont::PartitionedDataSet& partitionedDataSet,
  const viskores::Id firstNonEmptyPartitionId)
{
  viskores::Id numCells = 0;
  viskores::Id numPoints = 0;
  viskores::Id numOfDataSet = partitionedDataSet.GetNumberOfPartitions();
  std::vector<viskores::Id> cellOffsets(numOfDataSet);
  std::vector<viskores::Id> pointOffsets(numOfDataSet);
  //Mering cell set into single type
  //checking the cell type to make sure how many points per cell
  viskores::IdComponent numberOfPointsPerCell =
    partitionedDataSet.GetPartition(firstNonEmptyPartitionId)
      .GetCellSet()
      .GetNumberOfPointsInCell(0);
  for (viskores::Id partitionIndex = firstNonEmptyPartitionId; partitionIndex < numOfDataSet;
       partitionIndex++)
  {
    if (partitionedDataSet.GetPartition(partitionIndex).GetNumberOfPoints() == 0)
    {
      continue;
    }
    cellOffsets[partitionIndex] = numCells;
    numCells += partitionedDataSet.GetPartition(partitionIndex).GetNumberOfCells();
    pointOffsets[partitionIndex] = numPoints;
    numPoints += partitionedDataSet.GetPartition(partitionIndex).GetNumberOfPoints();
  }
  //We assume all cells have same type, which should have been previously checked.
  const viskores::Id mergedConnSize = numCells * numberOfPointsPerCell;
  // Calculating merged offsets for all domains
  viskores::cont::ArrayHandle<viskores::Id> mergedConn;
  mergedConn.Allocate(mergedConnSize);
  for (viskores::Id partitionIndex = firstNonEmptyPartitionId; partitionIndex < numOfDataSet;
       partitionIndex++)
  {
    if (partitionedDataSet.GetPartition(partitionIndex).GetNumberOfPoints() == 0)
    {
      continue;
    }
    auto cellSet = partitionedDataSet.GetPartition(partitionIndex).GetCellSet();
    // Grabing the connectivity and copy it into the larger connectivity array
    viskores::cont::CellSetSingleType<> singleType =
      cellSet.AsCellSet<viskores::cont::CellSetSingleType<>>();
    const viskores::cont::ArrayHandle<viskores::Id> connPerDataSet =
      singleType.GetConnectivityArray(viskores::TopologyElementTagCell(),
                                      viskores::TopologyElementTagPoint());
    viskores::Id copySize = connPerDataSet.GetNumberOfValues();
    VISKORES_ASSERT(copySize == cellSet.GetNumberOfCells() * numberOfPointsPerCell);
    // Mapping connectivity array per data into proper region of merged connectivity array
    // and also adjust the value in merged connectivity array
    viskores::cont::Invoker invoker;
    invoker(CopyWithOffsetWorklet{ pointOffsets[partitionIndex] },
            connPerDataSet,
            viskores::cont::make_ArrayHandleView(
              mergedConn, cellOffsets[partitionIndex] * numberOfPointsPerCell, copySize));
  }
  viskores::cont::CellSetSingleType<> cellSet;
  //Filling in the cellSet according to shapeId and number of points per cell.
  viskores::UInt8 cellShapeId =
    partitionedDataSet.GetPartition(firstNonEmptyPartitionId).GetCellSet().GetCellShape(0);
  cellSet.Fill(numPoints, cellShapeId, numberOfPointsPerCell, mergedConn);
  return cellSet;
}

viskores::cont::CellSetExplicit<> MergeCellSetsExplicit(
  const viskores::cont::PartitionedDataSet& partitionedDataSet,
  viskores::Id numPointsTotal,
  viskores::Id numCellsTotal,
  const viskores::Id firstNonEmptyPartitionId)
{
  viskores::cont::ArrayHandle<viskores::UInt8> shapes;
  viskores::cont::ArrayHandle<viskores::IdComponent> numIndices;
  MergeShapes(partitionedDataSet, numCellsTotal, shapes, numIndices, firstNonEmptyPartitionId);

  viskores::cont::ArrayHandle<viskores::Id> offsets;
  viskores::Id numIndicesTotal;
  viskores::cont::ConvertNumComponentsToOffsets(numIndices, offsets, numIndicesTotal);
  numIndices.ReleaseResources();

  //Merging connectivity/indicies array
  viskores::cont::ArrayHandle<viskores::Id> indices;
  MergeIndices(partitionedDataSet, offsets, numIndicesTotal, indices, firstNonEmptyPartitionId);

  viskores::cont::CellSetExplicit<> outCells;
  outCells.Fill(numPointsTotal, shapes, indices, offsets);
  return outCells;
}

viskores::Id GetFirstEmptyPartition(const viskores::cont::PartitionedDataSet& partitionedDataSet)
{
  viskores::Id numOfDataSet = partitionedDataSet.GetNumberOfPartitions();
  viskores::Id firstNonEmptyPartitionId = -1;
  for (viskores::Id partitionIndex = 0; partitionIndex < numOfDataSet; partitionIndex++)
  {
    if (partitionedDataSet.GetPartition(partitionIndex).GetNumberOfPoints() != 0)
    {
      firstNonEmptyPartitionId = partitionIndex;
      break;
    }
  }
  return firstNonEmptyPartitionId;
}

bool PartitionsAreSingleType(const viskores::cont::PartitionedDataSet partitionedDataSet,
                             const viskores::Id firstNonEmptyPartitionId)
{
  viskores::Id numOfDataSet = partitionedDataSet.GetNumberOfPartitions();
  for (viskores::Id partitionIndex = firstNonEmptyPartitionId; partitionIndex < numOfDataSet;
       partitionIndex++)
  {
    if (partitionedDataSet.GetPartition(partitionIndex).GetNumberOfPoints() == 0)
    {
      continue;
    }
    auto cellSet = partitionedDataSet.GetPartition(partitionIndex).GetCellSet();
    if (!cellSet.IsType<viskores::cont::CellSetSingleType<>>())
    {
      return false;
    }
  }

  //Make sure the cell type of each non-empty partition is same
  //with tested one, and they also have the same number of points.
  //We know that all cell sets are of type `CellSetSingleType<>` at this point.
  //Polygons may have different number of points even with the same shape id.
  //It is more efficient to `GetCellShape(0)` from `CellSetSingleType` compared with `CellSetExplicit`.
  auto cellSetFirst = partitionedDataSet.GetPartition(firstNonEmptyPartitionId).GetCellSet();
  viskores::UInt8 cellShapeId = cellSetFirst.GetCellShape(0);
  viskores::IdComponent numPointsInCell = cellSetFirst.GetNumberOfPointsInCell(0);
  for (viskores::Id partitionIndex = firstNonEmptyPartitionId + 1; partitionIndex < numOfDataSet;
       partitionIndex++)
  {
    auto cellSet = partitionedDataSet.GetPartition(partitionIndex).GetCellSet();
    if (cellSet.GetCellShape(0) != cellShapeId ||
        cellSet.GetNumberOfPointsInCell(0) != numPointsInCell)
    {
      return false;
    }
  }
  return true;
}

void CheckCoordsNames(const viskores::cont::PartitionedDataSet partitionedDataSet,
                      const viskores::Id firstNonEmptyPartitionId)
{
  viskores::IdComponent numCoords =
    partitionedDataSet.GetPartition(firstNonEmptyPartitionId).GetNumberOfCoordinateSystems();
  std::vector<std::string> coordsNames;
  for (viskores::IdComponent coordsIndex = 0; coordsIndex < numCoords; coordsIndex++)
  {
    coordsNames.push_back(partitionedDataSet.GetPartition(firstNonEmptyPartitionId)
                            .GetCoordinateSystemName(coordsIndex));
  }
  viskores::Id numOfDataSet = partitionedDataSet.GetNumberOfPartitions();
  for (viskores::Id partitionIndex = firstNonEmptyPartitionId; partitionIndex < numOfDataSet;
       partitionIndex++)
  {
    if (partitionedDataSet.GetPartition(partitionIndex).GetNumberOfPoints() == 0)
    {
      //Skip the empty data sets in the partitioned data sets
      continue;
    }
    if (numCoords != partitionedDataSet.GetPartition(partitionIndex).GetNumberOfCoordinateSystems())
    {
      throw viskores::cont::ErrorExecution("Data sets have different number of coordinate systems");
    }
    for (viskores::IdComponent coordsIndex = 0; coordsIndex < numCoords; coordsIndex++)
    {
      if (!partitionedDataSet.GetPartition(partitionIndex)
             .HasCoordinateSystem(coordsNames[coordsIndex]))
      {
        throw viskores::cont::ErrorExecution(
          "Coordinates system name: " + coordsNames[coordsIndex] +
          " in the first partition does not exist in other partitions");
      }
    }
  }
}


void MergeFieldsAndAddIntoDataSet(viskores::cont::DataSet& outputDataSet,
                                  const viskores::cont::PartitionedDataSet partitionedDataSet,
                                  const viskores::Id numPoints,
                                  const viskores::Id numCells,
                                  const viskores::Float64 invalidValue,
                                  const viskores::Id firstNonEmptyPartitionId)
{
  // Merging selected fields and coordinates
  // We get fields names in all partitions firstly
  // The inserted map stores the field name and a index of the first partition that owns that field
  viskores::cont::Invoker invoke;
  auto associationHash = [](viskores::cont::Field::Association const& association)
  { return static_cast<std::size_t>(association); };
  std::unordered_map<viskores::cont::Field::Association,
                     std::unordered_map<std::string, viskores::Id>,
                     decltype(associationHash)>
    fieldsMap(2, associationHash);

  viskores::Id numOfDataSet = partitionedDataSet.GetNumberOfPartitions();
  for (viskores::Id partitionIndex = firstNonEmptyPartitionId; partitionIndex < numOfDataSet;
       partitionIndex++)
  {
    if (partitionedDataSet.GetPartition(partitionIndex).GetNumberOfPoints() == 0)
    {
      continue;
    }
    viskores::IdComponent numFields =
      partitionedDataSet.GetPartition(partitionIndex).GetNumberOfFields();
    for (viskores::IdComponent fieldIndex = 0; fieldIndex < numFields; fieldIndex++)
    {
      const viskores::cont::Field& field =
        partitionedDataSet.GetPartition(partitionIndex).GetField(fieldIndex);
      auto association = field.GetAssociation();
      bool isSupported = (association == viskores::cont::Field::Association::Points ||
                          association == viskores::cont::Field::Association::Cells);
      if (!isSupported)
      {
        VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                       "Skipping merge of field '"
                         << field.GetName() << "' because it has an unsupported association.");
      }
      //Do not store the field index again if it exists in fieldMap
      if (fieldsMap[association].find(field.GetName()) != fieldsMap[association].end())
      {
        continue;
      }
      fieldsMap[association][field.GetName()] = partitionIndex;
    }
  }
  // Iterate all fields and create merged field arrays
  for (auto fieldMapIter = fieldsMap.begin(); fieldMapIter != fieldsMap.end(); ++fieldMapIter)
  {
    auto fieldAssociation = fieldMapIter->first;
    auto fieldNamesMap = fieldMapIter->second;
    for (auto fieldNameIter = fieldNamesMap.begin(); fieldNameIter != fieldNamesMap.end();
         ++fieldNameIter)
    {
      std::string fieldName = fieldNameIter->first;
      viskores::Id partitionOwnsField = fieldNameIter->second;
      const viskores::cont::Field& field =
        partitionedDataSet.GetPartition(partitionOwnsField).GetField(fieldName, fieldAssociation);

      viskores::cont::UnknownArrayHandle mergedFieldArray = field.GetData().NewInstanceBasic();
      if (fieldAssociation == viskores::cont::Field::Association::Points)
      {
        mergedFieldArray.Allocate(numPoints);
      }
      else
      {
        //We may add a new association (such as edges or faces) in future
        VISKORES_ASSERT(fieldAssociation == viskores::cont::Field::Association::Cells);
        mergedFieldArray.Allocate(numCells);
      }
      //Merging each field into the mergedField array
      auto resolveType = [&](auto& concreteOut)
      {
        viskores::Id offset = 0;
        for (viskores::Id partitionIndex = firstNonEmptyPartitionId; partitionIndex < numOfDataSet;
             ++partitionIndex)
        {
          if (partitionedDataSet.GetPartition(partitionIndex).GetNumberOfPoints() == 0)
          {
            continue;
          }
          if (partitionedDataSet.GetPartition(partitionIndex).HasField(fieldName, fieldAssociation))
          {
            viskores::cont::UnknownArrayHandle in = partitionedDataSet.GetPartition(partitionIndex)
                                                      .GetField(fieldName, fieldAssociation)
                                                      .GetData();
            viskores::Id copySize = in.GetNumberOfValues();
            auto viewOut = viskores::cont::make_ArrayHandleView(concreteOut, offset, copySize);
            viskores::cont::ArrayCopy(in, viewOut);
            offset += copySize;
          }
          else
          {
            //Creating invalid values for the partition that does not have the field
            using ComponentType =
              typename std::decay_t<decltype(concreteOut)>::ValueType::ComponentType;
            ComponentType castInvalid =
              viskores::cont::internal::CastInvalidValue<ComponentType>(invalidValue);
            viskores::Id copySize = 0;
            if (fieldAssociation == viskores::cont::Field::Association::Points)
            {
              copySize = partitionedDataSet.GetPartition(partitionIndex).GetNumberOfPoints();
            }
            else
            {
              copySize = partitionedDataSet.GetPartition(partitionIndex).GetNumberOfCells();
            }
            for (viskores::IdComponent component = 0;
                 component < concreteOut.GetNumberOfComponents();
                 ++component)
            {
              //Extracting each component from RecombineVec and copy invalid value into it
              //Avoid using invoke to call worklet on ArrayHandleRecombineVec (it may cause long compiling issue on CUDA 12.x).
              concreteOut.GetComponentArray(component).Fill(castInvalid, offset, offset + copySize);
            }
            offset += copySize;
          }
        }
      };
      mergedFieldArray.CastAndCallWithExtractedArray(resolveType);
      outputDataSet.AddField(viskores::cont::Field(fieldName, fieldAssociation, mergedFieldArray));
    }
  }
  return;
}

} // anonymous namespace

//-----------------------------------------------------------------------------

namespace viskores
{
namespace cont
{

VISKORES_CONT
viskores::cont::DataSet MergePartitionedDataSet(
  const viskores::cont::PartitionedDataSet& partitionedDataSet,
  viskores::Float64 invalidValue)
{
  viskores::cont::DataSet outputData;
  //The name of coordinates system in the first non-empty partition will be used in merged data set
  viskores::Id firstNonEmptyPartitionId = GetFirstEmptyPartition(partitionedDataSet);
  if (firstNonEmptyPartitionId == -1)
  {
    return outputData;
  }

  //Checking the name of coordinates system, if all partitions have different name with the firstNonEmptyPartitionId
  //just throw the exception now
  CheckCoordsNames(partitionedDataSet, firstNonEmptyPartitionId);

  //Checking if all partitions have CellSetSingleType with the same cell type
  bool allPartitionsAreSingleType =
    PartitionsAreSingleType(partitionedDataSet, firstNonEmptyPartitionId);

  viskores::Id numPointsTotal;
  viskores::Id numCellsTotal;
  CountPointsAndCells(partitionedDataSet, numPointsTotal, numCellsTotal);

  if (allPartitionsAreSingleType)
  {
    outputData.SetCellSet(MergeCellSetsSingleType(partitionedDataSet, firstNonEmptyPartitionId));
  }
  else
  {
    outputData.SetCellSet(MergeCellSetsExplicit(
      partitionedDataSet, numPointsTotal, numCellsTotal, firstNonEmptyPartitionId));
  }
  //Merging fields and coordinate systems
  MergeFieldsAndAddIntoDataSet(outputData,
                               partitionedDataSet,
                               numPointsTotal,
                               numCellsTotal,
                               invalidValue,
                               firstNonEmptyPartitionId);
  //Labeling fields that belong to the coordinate system.
  //There might be multiple coordinates systems, assuming all partitions have the same name of the coordinates system
  viskores::IdComponent numCoordsNames =
    partitionedDataSet.GetPartition(firstNonEmptyPartitionId).GetNumberOfCoordinateSystems();
  for (viskores::IdComponent i = 0; i < numCoordsNames; i++)
  {
    outputData.AddCoordinateSystem(
      partitionedDataSet.GetPartition(firstNonEmptyPartitionId).GetCoordinateSystemName(i));
  }
  return outputData;
}

}
}
