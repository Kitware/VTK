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
#ifndef viskores_cont_CellLocatorPartitioned_h
#define viskores_cont_CellLocatorPartitioned_h

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/exec/CellLocatorPartitioned.h>

namespace viskores
{
namespace cont
{
class VISKORES_CONT_EXPORT CellLocatorPartitioned : public viskores::cont::ExecutionObjectBase
{

public:
  virtual ~CellLocatorPartitioned() = default;

  VISKORES_CONT CellLocatorPartitioned() = default;

  void SetPartitions(const viskores::cont::PartitionedDataSet& partitions)
  {
    this->Partitions = partitions;
    this->SetModified();
  }
  const viskores::cont::PartitionedDataSet& GetPartitions() const { return this->Partitions; }

  void Update();

  void SetModified() { this->Modified = true; }
  bool GetModified() const { return this->Modified; }

  void Build();

  VISKORES_CONT const viskores::exec::CellLocatorPartitioned PrepareForExecution(
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token);

private:
  viskores::cont::PartitionedDataSet Partitions;
  std::vector<CellLocatorGeneral> LocatorsCont;
  std::vector<viskores::cont::ArrayHandleStride<viskores::UInt8>> GhostsCont;
  viskores::cont::ArrayHandle<viskores::cont::CellLocatorGeneral::ExecObjType> LocatorsExec;
  viskores::cont::ArrayHandle<viskores::cont::ArrayHandleStride<viskores::UInt8>::ReadPortalType>
    GhostsExec;
  bool Modified = true;
};
} // namespace cont
} //namespace viskores

#endif //viskores_cont_CellLocatorPartitioned_h
