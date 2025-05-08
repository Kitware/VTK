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

#include <viskores/Types.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/ErrorFilterExecution.h>

#include <viskores/filter/MapFieldPermutation.h>
#include <viskores/filter/contour/MIRFilter.h>
#include <viskores/filter/contour/worklet/MIR.h>

#include <viskores/filter/mesh_info/CellMeasures.h>

#include <viskores/worklet/Keys.h>
#include <viskores/worklet/ScatterCounting.h>

namespace viskores
{
namespace filter
{
namespace contour
{
VISKORES_CONT bool MIRFilter::DoMapField(
  viskores::cont::DataSet& result,
  const viskores::cont::Field& field,
  const viskores::cont::ArrayHandle<viskores::Id>& filterCellInterp,
  const viskores::cont::ArrayHandle<viskores::Vec<viskores::Float64, 8>>& MIRWeights,
  const viskores::cont::ArrayHandle<viskores::Vec<viskores::Id, 8>> MIRIDs)
{
  if (field.GetName().compare(this->pos_name) == 0 ||
      field.GetName().compare(this->len_name) == 0 || field.GetName().compare(this->id_name) == 0 ||
      field.GetName().compare(this->vf_name) == 0)
  {
    // Remember, we will map the field manually...
    // Technically, this will be for all of them...thus ignore it
    return false;
  }

  if (field.IsPointField())
  {
    viskores::cont::UnknownArrayHandle output = field.GetData().NewInstanceBasic();
    auto resolve = [&](const auto& concrete)
    {
      using T = typename std::decay_t<decltype(concrete)>::ValueType::ComponentType;
      auto outputArray = output.ExtractArrayFromComponents<T>(viskores::CopyFlag::Off);
      viskores::worklet::DestructPointWeightList destructWeightList;
      this->Invoke(destructWeightList, MIRIDs, MIRWeights, concrete, outputArray);
    };
    field.GetData().CastAndCallWithExtractedArray(resolve);
    result.AddPointField(field.GetName(), output);
    return true;
  }
  else if (field.IsCellField())
  {
    return viskores::filter::MapFieldPermutation(field, filterCellInterp, result);
  }
  else
  {
    return false;
  }
}

//-----------------------------------------------------------------------------
VISKORES_CONT viskores::cont::DataSet MIRFilter::DoExecute(const viskores::cont::DataSet& input)
{
  const viskores::cont::CoordinateSystem inputCoords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());
  viskores::cont::ArrayHandle<viskores::FloatDefault> avgSizeTot;
  viskores::filter::mesh_info::CellMeasures getSize;
  getSize.SetCellMeasureName("size");
  viskores::cont::ArrayCopyShallowIfPossible(getSize.Execute(input).GetCellField("size").GetData(),
                                             avgSizeTot);
  // First, load up all fields...
  viskores::cont::Field or_pos = input.GetField(this->pos_name);
  viskores::cont::Field or_len = input.GetField(this->len_name);
  viskores::cont::Field or_ids = input.GetField(this->id_name);
  viskores::cont::Field or_vfs = input.GetField(this->vf_name);
  // TODO: Check all fields for 'IsCellField'
  viskores::cont::ArrayHandle<viskores::FloatDefault> vfsdata_or, vfsdata;
  viskores::cont::ArrayHandle<viskores::Id> idsdata_or, idsdata, lendata_or, lendata, posdata_or,
    posdata, allids;
  or_pos.GetData().AsArrayHandle(posdata_or);
  or_len.GetData().AsArrayHandle(lendata_or);
  or_ids.GetData().AsArrayHandle(idsdata_or);
  or_vfs.GetData().AsArrayHandle(vfsdata_or);

  viskores::cont::ArrayCopy(idsdata_or, allids);
  viskores::cont::Algorithm::Sort(allids);
  viskores::cont::Algorithm::Unique(allids);
  viskores::IdComponent numIDs = static_cast<viskores::IdComponent>(allids.GetNumberOfValues());
  //using PortalConstType = viskores::cont::ArrayHandle<viskores::Id>::PortalConstControl;
  //PortalConstType readPortal = allids.GetPortalConstControl();
  using PortalConstType = viskores::cont::ArrayHandle<viskores::Id>::ReadPortalType;
  PortalConstType readPortal = allids.ReadPortal();
  viskores::cont::ArrayCopy(idsdata_or, idsdata);
  viskores::cont::ArrayCopy(lendata_or, lendata);
  viskores::cont::ArrayCopy(posdata_or, posdata);
  viskores::cont::ArrayCopy(vfsdata_or, vfsdata);
  //}

  viskores::cont::DataSet saved;
  // % error of the whole system, multiplied by the number of cells
  viskores::Float64 totalError = this->max_error + viskores::Float64(1.1); // Dummy value
  viskores::IdComponent currentIterationNum = 0;

  viskores::worklet::MIRCases::MIRTables faceTableArray;
  viskores::cont::ArrayHandle<viskores::Vec<viskores::Float64, 8>> pointWeights;
  viskores::cont::ArrayHandle<viskores::Vec<viskores::Id, 8>> pointIDs;
  viskores::worklet::ConstructCellWeightList constructReverseInformation;
  viskores::cont::ArrayHandleIndex pointCounter(input.GetNumberOfPoints());
  this->Invoke(constructReverseInformation, pointCounter, pointIDs, pointWeights);

  viskores::cont::ArrayHandle<viskores::Id> filterCellInterp;
  viskores::cont::ArrayHandle<viskores::Vec<viskores::Float64, 8>> MIRWeights;
  viskores::cont::ArrayHandle<viskores::Vec<viskores::Id, 8>> MIRIDs;

  do
  {
    saved = viskores::cont::DataSet();
    saved.AddCoordinateSystem(inputCoords);
    saved.SetCellSet(input.GetCellSet());

    viskores::cont::ArrayHandle<viskores::Id> currentcellIDs;
    viskores::cont::ArrayHandle<viskores::Id> pointlen, pointpos, pointid;
    viskores::cont::ArrayHandle<viskores::Float64> pointvf;
    viskores::worklet::CombineVFsForPoints_C convertOrigCellTo;
    viskores::worklet::CombineVFsForPoints convertOrigCellTo_Full;


    this->Invoke(convertOrigCellTo, saved.GetCellSet(), lendata, posdata, idsdata, pointlen);
    viskores::Id pointcount = viskores::cont::Algorithm::ScanExclusive(pointlen, pointpos);
    pointvf.Allocate(pointcount);
    pointid.Allocate(pointcount);
    this->Invoke(convertOrigCellTo_Full,
                 saved.GetCellSet(),
                 lendata,
                 posdata,
                 idsdata,
                 vfsdata,
                 pointpos,
                 pointid,
                 pointvf);

    viskores::worklet::MIRObject<viskores::Id, viskores::Float64> mirobj(
      pointlen, pointpos, pointid, pointvf); // This is point VF data...
    viskores::cont::ArrayHandle<viskores::Id> prevMat;
    viskores::cont::ArrayCopy(viskores::cont::make_ArrayHandleConstant<viskores::Id>(
                                -1, saved.GetCellSet().GetNumberOfCells()),
                              prevMat);
    viskores::cont::ArrayHandle<viskores::Id> cellLookback;
    viskores::cont::ArrayHandleIndex tmp_ind(saved.GetCellSet().GetNumberOfCells());
    viskores::cont::ArrayCopy(tmp_ind, cellLookback);
    viskores::IdComponent currentMatLoc = 0;

    while (currentMatLoc < numIDs)
    {
      viskores::IdComponent currentMatID =
        static_cast<viskores::IdComponent>(readPortal.Get(currentMatLoc++));
      if (currentMatID < 1)
      {
        VISKORES_LOG_S(
          viskores::cont::LogLevel::Fatal,
          "MIR filter does not accept materials with an non-positive ID! Material id in offense: "
            << currentMatID
            << ". Please remap all ID values to only positive numbers to avoid this issue.");
      }
      // First go through and pick out the previous and current material VFs for each cell.
      //{
      viskores::worklet::ExtractVFsForMIR_C extractCurrentMatVF;
      viskores::cont::ArrayHandle<viskores::Id> currentCellPointCounts;
      this->Invoke(extractCurrentMatVF, saved.GetCellSet(), currentCellPointCounts);
      viskores::worklet::ExtractVFsForMIR extractCurrentMatVF_SC(currentMatID);
      viskores::worklet::ScatterCounting extractCurrentMatVF_SC_scatter =
        extractCurrentMatVF_SC.MakeScatter(currentCellPointCounts);
      viskores::cont::ArrayHandle<viskores::Float64> currentMatVF;
      viskores::cont::ArrayHandle<viskores::Float64> previousMatVF;
      this->Invoke(extractCurrentMatVF_SC,
                   extractCurrentMatVF_SC_scatter,
                   saved.GetCellSet(),
                   mirobj,
                   prevMat,
                   currentMatVF,
                   previousMatVF);
      //}
      // Next see if we need to perform any work at all...
      if (currentMatLoc != 0)
      {
        // Run MIR, possibly changing colors...
        viskores::cont::ArrayHandle<viskores::Id> cellVFPointOffsets;
        viskores::cont::Algorithm::ScanExclusive(currentCellPointCounts, cellVFPointOffsets);
        viskores::worklet::MIR mir;
        viskores::cont::ArrayHandle<viskores::Id> newCellLookback, newCellID;

        viskores::cont::CellSetExplicit<> out = mir.Run(saved.GetCellSet(),
                                                        previousMatVF,
                                                        currentMatVF,
                                                        cellVFPointOffsets,
                                                        prevMat,
                                                        currentMatID,
                                                        cellLookback,
                                                        newCellID,
                                                        newCellLookback);
        viskores::cont::ArrayCopy(newCellLookback, cellLookback);
        viskores::cont::ArrayCopy(newCellID, prevMat);
        auto data = saved.GetCoordinateSystem(0).GetDataAsMultiplexer();
        auto coords = mir.ProcessPointField(data);
        // Now convert the point VFs...
        viskores::cont::ArrayHandle<viskores::Id> plen, ppos, pids;
        viskores::cont::ArrayHandle<viskores::Float64> pvf;
        mir.ProcessMIRField(mirobj.getPointLenArr(),
                            mirobj.getPointPosArr(),
                            mirobj.getPointIDArr(),
                            mirobj.getPointVFArr(),
                            plen,
                            ppos,
                            pids,
                            pvf);
        viskores::cont::ArrayHandle<viskores::Vec<viskores::Float64, 8>> tmppointWeights;
        viskores::cont::ArrayHandle<viskores::Vec<viskores::Id, 8>> tmppointIDs;
        mir.ProcessSimpleMIRField(pointIDs, pointWeights, tmppointIDs, tmppointWeights);
        viskores::cont::ArrayCopy(tmppointIDs, pointIDs);
        viskores::cont::ArrayCopy(tmppointWeights, pointWeights);
        //FileSaver fs;
        //fs(("pID" + std::to_string(currentMatID) + ".txt").c_str(), pointIDs);
        //fs(("wID" + std::to_string(currentMatID) + ".txt").c_str(), pointWeights);
        mirobj =
          viskores::worklet::MIRObject<viskores::Id, viskores::Float64>(plen, ppos, pids, pvf);
        saved = viskores::cont::DataSet();
        saved.SetCellSet(out);
        viskores::cont::CoordinateSystem outCo2(inputCoords.GetName(), coords);
        saved.AddCoordinateSystem(outCo2);
      }
    }


    // Hacking workaround to not clone an entire dataset.
    viskores::cont::ArrayHandle<viskores::FloatDefault> avgSize;
    viskores::cont::ArrayCopyShallowIfPossible(
      getSize.Execute(saved).GetCellField("size").GetData(), avgSize);

    viskores::worklet::CalcError_C calcErrC;
    viskores::worklet::Keys<viskores::Id> cellKeys(cellLookback);
    viskores::cont::ArrayCopy(cellLookback, filterCellInterp);
    viskores::cont::ArrayHandle<viskores::Id> lenOut, posOut, idsOut;
    viskores::cont::ArrayHandle<viskores::FloatDefault> vfsOut, totalErrorOut;

    lenOut.Allocate(cellKeys.GetUniqueKeys().GetNumberOfValues());
    this->Invoke(calcErrC, cellKeys, prevMat, lendata_or, posdata_or, idsdata_or, lenOut);

    viskores::Id numIDsOut = viskores::cont::Algorithm::ScanExclusive(lenOut, posOut);
    idsOut.Allocate(numIDsOut);
    vfsOut.Allocate(numIDsOut);
    viskores::worklet::CalcError calcErr(this->error_scaling);
    this->Invoke(calcErr,
                 cellKeys,
                 prevMat,
                 avgSize,
                 lendata_or,
                 posdata_or,
                 idsdata_or,
                 vfsdata_or,
                 lendata,
                 posdata,
                 idsdata,
                 vfsdata,
                 lenOut,
                 posOut,
                 idsOut,
                 vfsOut,
                 avgSizeTot,
                 totalErrorOut);

    totalError = viskores::cont::Algorithm::Reduce(totalErrorOut, viskores::Float64(0));
    viskores::cont::ArrayCopy(lenOut, lendata);
    viskores::cont::ArrayCopy(posOut, posdata);
    viskores::cont::ArrayCopy(idsOut, idsdata);
    viskores::cont::ArrayCopy(vfsOut, vfsdata);
    // Clean up the cells by calculating their volumes, and then calculate the relative error for each cell.
    // Note that the total error needs to be rescaled by the number of cells to get the % error.
    totalError = totalError / viskores::Float64(input.GetCellSet().GetNumberOfCells());
    this->error_scaling *= this->scaling_decay;

    VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                   "Mir iteration " << currentIterationNum + 1 << "/" << this->max_iter
                                    << "\t Total error: " << totalError);

    saved.AddField(viskores::cont::Field(
      this->GetOutputFieldName(), viskores::cont::Field::Association::Cells, prevMat));

    viskores::cont::ArrayCopy(pointIDs, MIRIDs);
    viskores::cont::ArrayCopy(pointWeights, MIRWeights);
  } while ((++currentIterationNum <= this->max_iter) && totalError >= this->max_error);

  auto mapper = [&](auto& outDataSet, const auto& f)
  { this->DoMapField(outDataSet, f, filterCellInterp, MIRWeights, MIRIDs); };
  auto output = this->CreateResultCoordinateSystem(
    input, saved.GetCellSet(), saved.GetCoordinateSystem(), mapper);
  output.AddField(saved.GetField(this->GetOutputFieldName()));

  return output;
}
} // namespace contour
} // namespace filter
} // namespace viskores
