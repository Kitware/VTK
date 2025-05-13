// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "ImageDataConverter.h"

#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/DataSetBuilderUniform.h>

#include "ArrayConverters.h"
#include "DataSetConverters.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkImageData.h"
#include "vtkPointData.h"

namespace
{

struct ComputeExtents
{
  template <viskores::IdComponent Dim>
  void operator()(const viskores::cont::CellSetStructured<Dim>& cs,
    const viskores::Id3& structuredCoordsDims, int extent[6]) const
  {
    auto extStart = cs.GetGlobalPointIndexStart();
    for (int i = 0, ii = 0; i < 3; ++i)
    {
      if (structuredCoordsDims[i] > 1)
      {
        extent[2 * i] = viskores::VecTraits<decltype(extStart)>::GetComponent(extStart, ii++);
        extent[(2 * i) + 1] = extent[2 * i] + structuredCoordsDims[i] - 1;
      }
      else
      {
        extent[2 * i] = extent[(2 * i) + 1] = 0;
      }
    }
  }
};

struct SetGlobalPointIndexStart
{
  template <viskores::IdComponent Dim>
  void operator()(const viskores::cont::CellSetStructured<Dim>&,
    const viskores::Id3& structuredCoordsDims, const int extent[6],
    viskores::cont::UnknownCellSet& dcs) const
  {
    typename viskores::cont::CellSetStructured<Dim>::SchedulingRangeType extStart{};
    for (int i = 0, ii = 0; i < 3; ++i)
    {
      if (structuredCoordsDims[i] > 1)
      {
        viskores::VecTraits<decltype(extStart)>::SetComponent(extStart, ii++, extent[2 * i]);
      }
    }

    viskores::cont::CellSetStructured<Dim> cs;
    dcs.AsCellSet(cs);
    cs.SetGlobalPointIndexStart(extStart);
  }
};

} // anonymous namespace

namespace tovtkm
{
VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// convert an image data type
viskores::cont::DataSet Convert(vtkImageData* input, FieldsFlag fields)
{
  int extent[6];
  input->GetExtent(extent);
  double vorigin[3];
  input->GetOrigin(vorigin);
  double vspacing[3];
  input->GetSpacing(vspacing);
  int vdims[3];
  input->GetDimensions(vdims);

  viskores::Vec<viskores::FloatDefault, 3> origin(
    static_cast<viskores::FloatDefault>(
      (static_cast<double>(extent[0]) * vspacing[0]) + vorigin[0]),
    static_cast<viskores::FloatDefault>(
      (static_cast<double>(extent[2]) * vspacing[1]) + vorigin[1]),
    static_cast<viskores::FloatDefault>(
      (static_cast<double>(extent[4]) * vspacing[2]) + vorigin[2]));
  viskores::Vec<viskores::FloatDefault, 3> spacing(static_cast<viskores::FloatDefault>(vspacing[0]),
    static_cast<viskores::FloatDefault>(vspacing[1]),
    static_cast<viskores::FloatDefault>(vspacing[2]));
  viskores::Id3 dims(vdims[0], vdims[1], vdims[2]);

  viskores::cont::DataSet dataset =
    viskores::cont::DataSetBuilderUniform::Create(dims, origin, spacing);

  using ListCellSetStructured = viskores::List<viskores::cont::CellSetStructured<1>,
    viskores::cont::CellSetStructured<2>, viskores::cont::CellSetStructured<3>>;
  auto cellSet = dataset.GetCellSet().ResetCellSetList(ListCellSetStructured{});
  viskores::cont::CastAndCall(
    cellSet, SetGlobalPointIndexStart{}, dims, extent, dataset.GetCellSet());

  ProcessFields(input, dataset, fields);

  return dataset;
}

VTK_ABI_NAMESPACE_END
} // tovtkm

namespace fromvtkm
{
VTK_ABI_NAMESPACE_BEGIN

bool Convert(
  const viskores::cont::DataSet& voutput, int extents[6], vtkImageData* output, vtkDataSet* input)
{
  viskores::cont::CoordinateSystem const& cs = voutput.GetCoordinateSystem();
  if (!cs.GetData().IsType<viskores::cont::ArrayHandleUniformPointCoordinates>())
  {
    return false;
  }

  auto points = cs.GetData().AsArrayHandle<viskores::cont::ArrayHandleUniformPointCoordinates>();
  auto portal = points.ReadPortal();

  auto origin = portal.GetOrigin();
  auto spacing = portal.GetSpacing();
  auto dim = portal.GetDimensions();
  VISKORES_ASSERT((extents[1] - extents[0] + 1) == dim[0] &&
    (extents[3] - extents[2] + 1) == dim[1] && (extents[5] - extents[4] + 1) == dim[2]);

  origin[0] -= static_cast<viskores::FloatDefault>(extents[0]) * spacing[0];
  origin[1] -= static_cast<viskores::FloatDefault>(extents[2]) * spacing[1];
  origin[2] -= static_cast<viskores::FloatDefault>(extents[4]) * spacing[2];

  output->SetExtent(extents);
  output->SetOrigin(origin[0], origin[1], origin[2]);
  output->SetSpacing(spacing[0], spacing[1], spacing[2]);

  // Next we need to convert any extra fields from viskores over to vtk
  bool arraysConverted = fromvtkm::ConvertArrays(voutput, output);

  // Pass information about attributes.
  PassAttributesInformation(input->GetPointData(), output->GetPointData());
  PassAttributesInformation(input->GetCellData(), output->GetCellData());

  return arraysConverted;
}

bool Convert(const viskores::cont::DataSet& voutput, vtkImageData* output, vtkDataSet* input)
{
  viskores::cont::CoordinateSystem const& cs = voutput.GetCoordinateSystem();
  if (!cs.GetData().IsType<viskores::cont::ArrayHandleUniformPointCoordinates>())
  {
    return false;
  }

  auto points = cs.GetData().AsArrayHandle<viskores::cont::ArrayHandleUniformPointCoordinates>();
  auto portal = points.ReadPortal();

  auto dim = portal.GetDimensions();
  int extents[6];
  using ListCellSetStructured = viskores::List<viskores::cont::CellSetStructured<1>,
    viskores::cont::CellSetStructured<2>, viskores::cont::CellSetStructured<3>>;
  auto cellSet = voutput.GetCellSet().ResetCellSetList(ListCellSetStructured{});
  viskores::cont::CastAndCall(cellSet, ComputeExtents{}, dim, extents);

  return Convert(voutput, extents, output, input);
}

VTK_ABI_NAMESPACE_END
} // fromvtkm
