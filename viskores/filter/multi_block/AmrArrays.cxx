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
#include <viskores/CellClassification.h>
#include <viskores/RangeId.h>
#include <viskores/Types.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/BoundsCompute.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/filter/multi_block/AmrArrays.h>

namespace viskores
{
namespace worklet
{

// this worklet sets the blanked bit to one if the cell has an overlap with a cell in one of its children
template <viskores::IdComponent Dim>
struct GenerateGhostTypeWorklet : viskores::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cellSet,
                                FieldInPoint pointArray,
                                FieldInOutCell ghostArray);
  using ExecutionSignature = void(PointCount, _2, _3);
  using InputDomain = _1;

  GenerateGhostTypeWorklet(viskores::Bounds boundsChild)
    : BoundsChild(boundsChild)
  {
  }

  template <typename pointArrayType, typename cellArrayType>
  VISKORES_EXEC void operator()(viskores::IdComponent numPoints,
                                const pointArrayType pointArray,
                                cellArrayType& ghostArray) const
  {
    viskores::Bounds boundsCell = viskores::Bounds();
    for (viskores::IdComponent pointId = 0; pointId < numPoints; pointId++)
    {
      boundsCell.Include(pointArray[pointId]);
    }
    viskores::Bounds boundsIntersection = boundsCell.Intersection(BoundsChild);
    if ((Dim == 2 && boundsIntersection.Area() > 0.5 * boundsCell.Area()) ||
        (Dim == 3 && boundsIntersection.Volume() > 0.5 * boundsCell.Volume()))
    {
      //      std::cout<<boundsCell<<" is (partly) contained in "<<BoundsChild<<" "<<boundsIntersection<<" "<<boundsIntersection.Area()<<std::endl;
      ghostArray = ghostArray | viskores::CellClassification::Blanked;
    }
  }

  viskores::Bounds BoundsChild;
};

// this worklet sets the blanked bit to zero,
// it discards all old information on blanking whil keeping the other bits, e.g., for ghost cells
struct ResetGhostTypeWorklet : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn ghostArrayIn, FieldOut ghostArrayOut);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;
  VISKORES_EXEC void operator()(UInt8 ghostArrayIn, UInt8& ghostArrayOut) const
  {
    ghostArrayOut = ghostArrayIn & ~viskores::CellClassification::Blanked;
  }
};

} // worklet
} // viskores

namespace viskores
{
namespace filter
{
namespace multi_block
{
VISKORES_CONT
void AmrArrays::GenerateParentChildInformation()
{
  viskores::Bounds bounds = viskores::cont::BoundsCompute(this->AmrDataSet);
  if (bounds.Z.Max - bounds.Z.Min < viskores::Epsilon<viskores::FloatDefault>())
  {
    ComputeGenerateParentChildInformation<2>();
  }
  else
  {
    ComputeGenerateParentChildInformation<3>();
  }
}

template void AmrArrays::ComputeGenerateParentChildInformation<2>();

template void AmrArrays::ComputeGenerateParentChildInformation<3>();

VISKORES_CONT
template <viskores::IdComponent Dim>
void AmrArrays::ComputeGenerateParentChildInformation()
{
  // read out spacings in decreasing order to infer levels
  std::set<FloatDefault, std::greater<FloatDefault>> spacings;
  for (viskores::Id p = 0; p < this->AmrDataSet.GetNumberOfPartitions(); p++)
  {
    viskores::cont::ArrayHandleUniformPointCoordinates uniformCoords =
      this->AmrDataSet.GetPartition(p)
        .GetCoordinateSystem()
        .GetData()
        .AsArrayHandle<viskores::cont::ArrayHandleUniformPointCoordinates>();
    spacings.insert(uniformCoords.GetSpacing()[0]);
  }
  std::set<FloatDefault, std::greater<FloatDefault>>::iterator itr;

  /// contains the partitionIds of each level and blockId
  this->PartitionIds.resize(spacings.size());
  for (viskores::Id p = 0; p < this->AmrDataSet.GetNumberOfPartitions(); p++)
  {
    viskores::cont::ArrayHandleUniformPointCoordinates uniformCoords =
      this->AmrDataSet.GetPartition(p)
        .GetCoordinateSystem()
        .GetData()
        .AsArrayHandle<viskores::cont::ArrayHandleUniformPointCoordinates>();
    int index = -1;
    for (itr = spacings.begin(); itr != spacings.end(); itr++)
    {
      index++;
      if (*itr == uniformCoords.GetSpacing()[0])
      {
        break;
      }
    }
    this->PartitionIds.at(index).push_back(p);
  }

  // compute parent and child relations
  this->ParentsIdsVector.resize(this->AmrDataSet.GetNumberOfPartitions());
  this->ChildrenIdsVector.resize(this->AmrDataSet.GetNumberOfPartitions());
  for (unsigned int l = 0; l < this->PartitionIds.size() - 1; l++)
  {
    for (unsigned int bParent = 0; bParent < this->PartitionIds.at(l).size(); bParent++)
    {
      //        std::cout << std::endl << "level " << l << " block " << bParent << std::endl;
      viskores::Bounds boundsParent =
        this->AmrDataSet.GetPartition(this->PartitionIds.at(l).at(bParent))
          .GetCoordinateSystem()
          .GetBounds();

      // compute size of a cell to compare overlap against
      auto coords = this->AmrDataSet.GetPartition(this->PartitionIds.at(l).at(bParent))
                      .GetCoordinateSystem()
                      .GetDataAsMultiplexer();
      viskores::cont::CellSetStructured<Dim> cellset;
      viskores::Id ptids[8];
      this->AmrDataSet.GetPartition(this->PartitionIds.at(l).at(bParent))
        .GetCellSet()
        .AsCellSet(cellset);
      cellset.GetCellPointIds(0, ptids);
      viskores::Bounds boundsCell = viskores::Bounds();
      for (viskores::IdComponent pointId = 0; pointId < cellset.GetNumberOfPointsInCell(0);
           pointId++)
      {
        boundsCell.Include(coords.ReadPortal().Get(ptids[pointId]));
      }

      // see if there is overlap of at least one half of a cell
      for (unsigned int bChild = 0; bChild < this->PartitionIds.at(l + 1).size(); bChild++)
      {
        viskores::Bounds boundsChild =
          this->AmrDataSet.GetPartition(this->PartitionIds.at(l + 1).at(bChild))
            .GetCoordinateSystem()
            .GetBounds();
        viskores::Bounds boundsIntersection = boundsParent.Intersection(boundsChild);
        if ((Dim == 2 && boundsIntersection.Area() > 0.5 * boundsCell.Area()) ||
            (Dim == 3 && boundsIntersection.Volume() >= 0.5 * boundsCell.Volume()))
        {
          this->ParentsIdsVector.at(this->PartitionIds.at(l + 1).at(bChild))
            .push_back(this->PartitionIds.at(l).at(bParent));
          this->ChildrenIdsVector.at(this->PartitionIds.at(l).at(bParent))
            .push_back(this->PartitionIds.at(l + 1).at(bChild));
          //          std::cout << " overlaps with level " << l + 1 << " block  " << bChild << " "
          //                    << boundsParent << " " << boundsChild << " " << boundsIntersection << " "
          //          << boundsIntersection.Area() << " " << boundsIntersection.Volume() << std::endl;
        }
      }
    }
  }
}

VISKORES_CONT
void AmrArrays::GenerateGhostType()
{
  viskores::Bounds bounds = viskores::cont::BoundsCompute(this->AmrDataSet);
  if (bounds.Z.Max - bounds.Z.Min < viskores::Epsilon<viskores::FloatDefault>())
  {
    ComputeGenerateGhostType<2>();
  }
  else
  {
    ComputeGenerateGhostType<3>();
  }
}

template void AmrArrays::ComputeGenerateGhostType<2>();

template void AmrArrays::ComputeGenerateGhostType<3>();

VISKORES_CONT
template <viskores::IdComponent Dim>
void AmrArrays::ComputeGenerateGhostType()
{
  for (unsigned int l = 0; l < this->PartitionIds.size(); l++)
  {
    for (unsigned int bParent = 0; bParent < this->PartitionIds.at(l).size(); bParent++)
    {
      //      std::cout<<std::endl<<"level  "<<l<<" block  "<<bParent<<" has  "<<this->ChildrenIdsVector.at(this->PartitionIds.at(l).at(bParent)).size()<<" children"<<std::endl;

      viskores::cont::DataSet partition =
        this->AmrDataSet.GetPartition(this->PartitionIds.at(l).at(bParent));
      viskores::cont::CellSetStructured<Dim> cellset;
      partition.GetCellSet().AsCellSet(cellset);
      viskores::cont::ArrayHandle<viskores::UInt8> ghostArrayHandle;
      if (!partition.HasGhostCellField())
      {
        ghostArrayHandle.AllocateAndFill(partition.GetNumberOfCells(), 0);
      }
      else
      {
        viskores::cont::Invoker invoke;
        invoke(viskores::worklet::ResetGhostTypeWorklet{},
               partition.GetGhostCellField()
                 .GetData()
                 .AsArrayHandle<viskores::cont::ArrayHandle<viskores::UInt8>>(),
               ghostArrayHandle);
      }
      partition.AddCellField(viskores::cont::GetGlobalGhostCellFieldName(), ghostArrayHandle);

      auto pointField = partition.GetCoordinateSystem().GetDataAsMultiplexer();

      for (unsigned int bChild = 0;
           bChild < this->ChildrenIdsVector.at(this->PartitionIds.at(l).at(bParent)).size();
           bChild++)
      {
        viskores::Bounds boundsChild =
          this->AmrDataSet
            .GetPartition(
              this->ChildrenIdsVector.at(this->PartitionIds.at(l).at(bParent)).at(bChild))
            .GetCoordinateSystem()
            .GetBounds();
        //        std::cout<<" is (partly) contained in level "<<l + 1<<" block "<<bChild<<" which is partition "<<this->ChildrenIdsVector.at(this->PartitionIds.at(l).at(bParent)).at(bChild)<<" with bounds "<<boundsChild<<std::endl;

        viskores::cont::Invoker invoke;
        invoke(viskores::worklet::GenerateGhostTypeWorklet<Dim>{ boundsChild },
               cellset,
               pointField,
               ghostArrayHandle);
      }
      this->AmrDataSet.ReplacePartition(this->PartitionIds.at(l).at(bParent), partition);
    }
  }
}

// Add helper arrays like in ParaView
VISKORES_CONT
void AmrArrays::GenerateIndexArrays()
{
  for (unsigned int l = 0; l < this->PartitionIds.size(); l++)
  {
    for (unsigned int b = 0; b < this->PartitionIds.at(l).size(); b++)
    {
      viskores::cont::DataSet partition =
        this->AmrDataSet.GetPartition(this->PartitionIds.at(l).at(b));

      viskores::cont::ArrayHandle<viskores::Id> fieldAmrLevel;
      viskores::cont::ArrayCopy(
        viskores::cont::ArrayHandleConstant<viskores::Id>(l, partition.GetNumberOfCells()),
        fieldAmrLevel);
      partition.AddCellField("vtkAmrLevel", fieldAmrLevel);

      viskores::cont::ArrayHandle<viskores::Id> fieldBlockId;
      viskores::cont::ArrayCopy(
        viskores::cont::ArrayHandleConstant<viskores::Id>(b, partition.GetNumberOfCells()),
        fieldBlockId);
      partition.AddCellField("vtkAmrIndex", fieldBlockId);

      viskores::cont::ArrayHandle<viskores::Id> fieldPartitionIndex;
      viskores::cont::ArrayCopy(viskores::cont::ArrayHandleConstant<viskores::Id>(
                                  this->PartitionIds.at(l).at(b), partition.GetNumberOfCells()),
                                fieldPartitionIndex);
      partition.AddCellField("vtkCompositeIndex", fieldPartitionIndex);

      this->AmrDataSet.ReplacePartition(this->PartitionIds.at(l).at(b), partition);
    }
  }
}

viskores::cont::PartitionedDataSet AmrArrays::DoExecutePartitions(
  const viskores::cont::PartitionedDataSet& input)
{
  this->AmrDataSet = input;
  this->GenerateParentChildInformation();
  this->GenerateGhostType();
  this->GenerateIndexArrays();
  return this->AmrDataSet;
}
} // namespace multi_block
} // namespace filter
} // namespace viskores
