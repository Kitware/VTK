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

#ifndef viskores_worklet_Gradient_h
#define viskores_worklet_Gradient_h

#include <viskores/worklet/DispatcherMapTopology.h>
#include <viskores/worklet/DispatcherPointNeighborhood.h>

#include <viskores/filter/vector_analysis/worklet/gradient/CellGradient.h>
#include <viskores/filter/vector_analysis/worklet/gradient/Divergence.h>
#include <viskores/filter/vector_analysis/worklet/gradient/GradientOutput.h>
#include <viskores/filter/vector_analysis/worklet/gradient/PointGradient.h>
#include <viskores/filter/vector_analysis/worklet/gradient/QCriterion.h>
#include <viskores/filter/vector_analysis/worklet/gradient/StructuredPointGradient.h>
#include <viskores/filter/vector_analysis/worklet/gradient/Transpose.h>
#include <viskores/filter/vector_analysis/worklet/gradient/Vorticity.h>

// Required for instantiations
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCartesianProduct.h>
#include <viskores/cont/ArrayHandleSOA.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/UnknownCellSet.h>
#include <viskores/internal/Instantiations.h>

#ifndef viskores_GradientInstantiation
// Turn this on to check to see if all the instances of the gradient worklet are covered
// in external instances. If they are not, you will get a compile error.
//#define VISKORES_GRADIENT_CHECK_WORKLET_INSTANCES
#endif

namespace viskores
{
namespace worklet
{

template <typename T>
struct GradientOutputFields;

namespace gradient
{

//-----------------------------------------------------------------------------
template <typename CoordinateSystem, typename T, typename S>
struct DeducedPointGrad
{
  DeducedPointGrad(const CoordinateSystem& coords,
                   const viskores::cont::ArrayHandle<T, S>& field,
                   GradientOutputFields<T>* result)
    : Points(&coords)
    , Field(&field)
    , Result(result)
  {
  }

  template <typename CellSetType>
  void operator()(const CellSetType& cellset) const;

  template <typename CellSetType>
  void Go(const CellSetType& cellset) const
  {
    viskores::worklet::DispatcherMapTopology<PointGradient> dispatcher;
    dispatcher.Invoke(cellset, //topology to iterate on a per point basis
                      cellset, //whole cellset in
                      *this->Points,
                      *this->Field,
                      *this->Result);
  }

  void Go(const viskores::cont::CellSetStructured<3>& cellset) const
  {
    viskores::worklet::DispatcherPointNeighborhood<StructuredPointGradient> dispatcher;
    dispatcher.Invoke(cellset, //topology to iterate on a per point basis
                      *this->Points,
                      *this->Field,
                      *this->Result);
  }

  template <typename PermIterType>
  void Go(const viskores::cont::CellSetPermutation<viskores::cont::CellSetStructured<3>,
                                                   PermIterType>& cellset) const
  {
    viskores::worklet::DispatcherPointNeighborhood<StructuredPointGradient> dispatcher;
    dispatcher.Invoke(cellset, //topology to iterate on a per point basis
                      *this->Points,
                      *this->Field,
                      *this->Result);
  }

  void Go(const viskores::cont::CellSetStructured<2>& cellset) const
  {
    viskores::worklet::DispatcherPointNeighborhood<StructuredPointGradient> dispatcher;
    dispatcher.Invoke(cellset, //topology to iterate on a per point basis
                      *this->Points,
                      *this->Field,
                      *this->Result);
  }

  template <typename PermIterType>
  void Go(const viskores::cont::CellSetPermutation<viskores::cont::CellSetStructured<2>,
                                                   PermIterType>& cellset) const
  {
    viskores::worklet::DispatcherPointNeighborhood<StructuredPointGradient> dispatcher;
    dispatcher.Invoke(cellset, //topology to iterate on a per point basis
                      *this->Points,
                      *this->Field,
                      *this->Result);
  }


  const CoordinateSystem* const Points;
  const viskores::cont::ArrayHandle<T, S>* const Field;
  GradientOutputFields<T>* Result;

private:
  void operator=(const DeducedPointGrad<CoordinateSystem, T, S>&) = delete;
};

#ifndef VISKORES_GRADIENT_CHECK_WORKLET_INSTANCES
// Declare the methods that get instances outside of the class so that they are not inline.
// If they are inline, the compiler may decide to compile them anyway.
template <typename CoordinateSystem, typename T, typename S>
template <typename CellSetType>
void DeducedPointGrad<CoordinateSystem, T, S>::operator()(const CellSetType& cellset) const
{
  this->Go(cellset);
}
#endif

} //namespace gradient

template <typename T>
struct GradientOutputFields : public viskores::cont::ExecutionObjectBase
{

  using ValueType = T;
  using BaseTType = typename viskores::VecTraits<T>::BaseComponentType;

  template <typename DeviceAdapter>
  struct ExecutionTypes
  {
    using Portal = viskores::exec::GradientOutput<T>;
  };

  GradientOutputFields()
    : Gradient()
    , Divergence()
    , Vorticity()
    , QCriterion()
    , StoreGradient(true)
    , ComputeDivergence(false)
    , ComputeVorticity(false)
    , ComputeQCriterion(false)
  {
  }

  GradientOutputFields(bool store, bool divergence, bool vorticity, bool qc)
    : Gradient()
    , Divergence()
    , Vorticity()
    , QCriterion()
    , StoreGradient(store)
    , ComputeDivergence(divergence)
    , ComputeVorticity(vorticity)
    , ComputeQCriterion(qc)
  {
  }

  /// Add divergence field to the output data.
  /// The input array must have 3 components in order to compute this.
  /// The default is off.
  void SetComputeDivergence(bool enable) { ComputeDivergence = enable; }
  bool GetComputeDivergence() const { return ComputeDivergence; }

  /// Add voriticity/curl field to the output data.
  /// The input array must have 3 components in order to compute this.
  /// The default is off.
  void SetComputeVorticity(bool enable) { ComputeVorticity = enable; }
  bool GetComputeVorticity() const { return ComputeVorticity; }

  /// Add Q-criterion field to the output data.
  /// The input array must have 3 components in order to compute this.
  /// The default is off.
  void SetComputeQCriterion(bool enable) { ComputeQCriterion = enable; }
  bool GetComputeQCriterion() const { return ComputeQCriterion; }

  /// Add gradient field to the output data.
  /// The input array must have 3 components in order to disable this.
  /// The default is on.
  void SetComputeGradient(bool enable) { StoreGradient = enable; }
  bool GetComputeGradient() const { return StoreGradient; }

  //todo fix this for scalar
  viskores::exec::GradientOutput<T> PrepareForOutput(viskores::Id size)
  {
    viskores::exec::GradientOutput<T> portal(this->StoreGradient,
                                             this->ComputeDivergence,
                                             this->ComputeVorticity,
                                             this->ComputeQCriterion,
                                             this->Gradient,
                                             this->Divergence,
                                             this->Vorticity,
                                             this->QCriterion,
                                             size);
    return portal;
  }

  viskores::cont::ArrayHandle<viskores::Vec<T, 3>> Gradient;
  viskores::cont::ArrayHandle<BaseTType> Divergence;
  viskores::cont::ArrayHandle<viskores::Vec<BaseTType, 3>> Vorticity;
  viskores::cont::ArrayHandle<BaseTType> QCriterion;

private:
  bool StoreGradient;
  bool ComputeDivergence;
  bool ComputeVorticity;
  bool ComputeQCriterion;
};
class PointGradient
{
public:
  template <typename CellSetType, typename CoordinateSystem, typename T, typename S>
  viskores::cont::ArrayHandle<viskores::Vec<T, 3>> Run(
    const CellSetType& cells,
    const CoordinateSystem& coords,
    const viskores::cont::ArrayHandle<T, S>& field,
    GradientOutputFields<T>& extraOutput)
  {
    //we are using cast and call here as we pass the cells twice to the invoke
    //and want the type resolved once before hand instead of twice
    //by the dispatcher ( that will cost more in time and binary size )
    gradient::DeducedPointGrad<CoordinateSystem, T, S> func(coords, field, &extraOutput);
    viskores::cont::CastAndCall(cells, func);
    return extraOutput.Gradient;
  }
};

class CellGradient
{
public:
  template <typename CellSetType, typename CoordinateSystem, typename T, typename S>
  static viskores::cont::ArrayHandle<viskores::Vec<T, 3>> Run(
    const CellSetType& cells,
    const CoordinateSystem& coords,
    const viskores::cont::ArrayHandle<T, S>& field,
    GradientOutputFields<T>& extraOutput);
};

#ifndef VISKORES_GRADIENT_CHECK_WORKLET_INSTANCES
// Declare the methods that get instances outside of the class so that they are not inline.
// If they are inline, the compiler may decide to compile them anyway.
template <typename CellSetType, typename CoordinateSystem, typename T, typename S>
viskores::cont::ArrayHandle<viskores::Vec<T, 3>> CellGradient::Run(
  const CellSetType& cells,
  const CoordinateSystem& coords,
  const viskores::cont::ArrayHandle<T, S>& field,
  GradientOutputFields<T>& extraOutput)
{
  viskores::worklet::DispatcherMapTopology<viskores::worklet::gradient::CellGradient> dispatcher;
  dispatcher.Invoke(cells, coords, field, extraOutput);
  return extraOutput.Gradient;
}
#endif

}
} // namespace viskores::worklet

//==============================================================================
//---------------------------------------------------------------------------
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Float32,
  viskores::cont::StorageTagBasic>::operator()(const viskores::cont::CellSetStructured<3>&) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Float64,
  viskores::cont::StorageTagBasic>::operator()(const viskores::cont::CellSetStructured<3>&) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f_32,
  viskores::cont::StorageTagBasic>::operator()(const viskores::cont::CellSetStructured<3>&) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f_64,
  viskores::cont::StorageTagBasic>::operator()(const viskores::cont::CellSetStructured<3>&) const;
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f_32,
  viskores::cont::StorageTagSOA>::operator()(const viskores::cont::CellSetStructured<3>&) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f_64,
  viskores::cont::StorageTagSOA>::operator()(const viskores::cont::CellSetStructured<3>&) const;
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f_32,
  viskores::cont::StorageTagCartesianProduct<viskores::cont::StorageTagBasic,
                                             viskores::cont::StorageTagBasic,
                                             viskores::cont::StorageTagBasic>>::
operator()(const viskores::cont::CellSetStructured<3>&) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f_64,
  viskores::cont::StorageTagCartesianProduct<viskores::cont::StorageTagBasic,
                                             viskores::cont::StorageTagBasic,
                                             viskores::cont::StorageTagBasic>>::
operator()(const viskores::cont::CellSetStructured<3>&) const;
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f,
  viskores::cont::StorageTagUniformPoints>::operator()(const viskores::cont::CellSetStructured<3>&)
  const;
VISKORES_INSTANTIATION_END


//---------------------------------------------------------------------------
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Float32,
  viskores::cont::StorageTagBasic>::operator()(const viskores::cont::CellSetStructured<2>&) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Float64,
  viskores::cont::StorageTagBasic>::operator()(const viskores::cont::CellSetStructured<2>&) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f_32,
  viskores::cont::StorageTagBasic>::operator()(const viskores::cont::CellSetStructured<2>&) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f_64,
  viskores::cont::StorageTagBasic>::operator()(const viskores::cont::CellSetStructured<2>&) const;
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f_32,
  viskores::cont::StorageTagSOA>::operator()(const viskores::cont::CellSetStructured<2>&) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f_64,
  viskores::cont::StorageTagSOA>::operator()(const viskores::cont::CellSetStructured<2>&) const;
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f_32,
  viskores::cont::StorageTagCartesianProduct<viskores::cont::StorageTagBasic,
                                             viskores::cont::StorageTagBasic,
                                             viskores::cont::StorageTagBasic>>::
operator()(const viskores::cont::CellSetStructured<2>&) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f_64,
  viskores::cont::StorageTagCartesianProduct<viskores::cont::StorageTagBasic,
                                             viskores::cont::StorageTagBasic,
                                             viskores::cont::StorageTagBasic>>::
operator()(const viskores::cont::CellSetStructured<2>&) const;
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f,
  viskores::cont::StorageTagUniformPoints>::operator()(const viskores::cont::CellSetStructured<2>&)
  const;
VISKORES_INSTANTIATION_END


//---------------------------------------------------------------------------
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Float32,
  viskores::cont::StorageTagBasic>::operator()(const viskores::cont::CellSetExplicit<>&) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Float64,
  viskores::cont::StorageTagBasic>::operator()(const viskores::cont::CellSetExplicit<>&) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f_32,
  viskores::cont::StorageTagBasic>::operator()(const viskores::cont::CellSetExplicit<>&) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f_64,
  viskores::cont::StorageTagBasic>::operator()(const viskores::cont::CellSetExplicit<>&) const;
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f_32,
  viskores::cont::StorageTagSOA>::operator()(const viskores::cont::CellSetExplicit<>&) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f_64,
  viskores::cont::StorageTagSOA>::operator()(const viskores::cont::CellSetExplicit<>&) const;
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f_32,
  viskores::cont::StorageTagCartesianProduct<viskores::cont::StorageTagBasic,
                                             viskores::cont::StorageTagBasic,
                                             viskores::cont::StorageTagBasic>>::
operator()(const viskores::cont::CellSetExplicit<>&) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f_64,
  viskores::cont::StorageTagCartesianProduct<viskores::cont::StorageTagBasic,
                                             viskores::cont::StorageTagBasic,
                                             viskores::cont::StorageTagBasic>>::
operator()(const viskores::cont::CellSetExplicit<>&) const;
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f,
  viskores::cont::StorageTagUniformPoints>::operator()(const viskores::cont::CellSetExplicit<>&)
  const;
VISKORES_INSTANTIATION_END


//---------------------------------------------------------------------------
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Float32,
  viskores::cont::StorageTagBasic>::operator()(const viskores::cont::CellSetSingleType<>&) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Float64,
  viskores::cont::StorageTagBasic>::operator()(const viskores::cont::CellSetSingleType<>&) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f_32,
  viskores::cont::StorageTagBasic>::operator()(const viskores::cont::CellSetSingleType<>&) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f_64,
  viskores::cont::StorageTagBasic>::operator()(const viskores::cont::CellSetSingleType<>&) const;
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f_32,
  viskores::cont::StorageTagSOA>::operator()(const viskores::cont::CellSetSingleType<>&) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f_64,
  viskores::cont::StorageTagSOA>::operator()(const viskores::cont::CellSetSingleType<>&) const;
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f_32,
  viskores::cont::StorageTagCartesianProduct<viskores::cont::StorageTagBasic,
                                             viskores::cont::StorageTagBasic,
                                             viskores::cont::StorageTagBasic>>::
operator()(const viskores::cont::CellSetSingleType<>&) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f_64,
  viskores::cont::StorageTagCartesianProduct<viskores::cont::StorageTagBasic,
                                             viskores::cont::StorageTagBasic,
                                             viskores::cont::StorageTagBasic>>::
operator()(const viskores::cont::CellSetSingleType<>&) const;
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::gradient::DeducedPointGrad<
  viskores::cont::CoordinateSystem,
  viskores::Vec3f,
  viskores::cont::StorageTagUniformPoints>::operator()(const viskores::cont::CellSetSingleType<>&)
  const;
VISKORES_INSTANTIATION_END



//==============================================================================
//---------------------------------------------------------------------------
VISKORES_INSTANTIATION_BEGIN
extern template viskores::cont::ArrayHandle<viskores::Vec<viskores::Float32, 3>>
viskores::worklet::CellGradient::Run(
  const viskores::cont::UnknownCellSet&,
  const viskores::cont::CoordinateSystem&,
  const viskores::cont::ArrayHandle<viskores::Float32, viskores::cont::StorageTagBasic>&,
  GradientOutputFields<viskores::Float32>&);
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template viskores::cont::ArrayHandle<viskores::Vec<viskores::Float64, 3>>
viskores::worklet::CellGradient::Run(
  const viskores::cont::UnknownCellSet&,
  const viskores::cont::CoordinateSystem&,
  const viskores::cont::ArrayHandle<viskores::Float64, viskores::cont::StorageTagBasic>&,
  GradientOutputFields<viskores::Float64>&);
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template viskores::cont::ArrayHandle<viskores::Vec<viskores::Vec3f_32, 3>>
viskores::worklet::CellGradient::Run(
  const viskores::cont::UnknownCellSet&,
  const viskores::cont::CoordinateSystem&,
  const viskores::cont::ArrayHandle<viskores::Vec3f_32, viskores::cont::StorageTagBasic>&,
  GradientOutputFields<viskores::Vec3f_32>&);
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template viskores::cont::ArrayHandle<viskores::Vec<viskores::Vec3f_64, 3>>
viskores::worklet::CellGradient::Run(
  const viskores::cont::UnknownCellSet&,
  const viskores::cont::CoordinateSystem&,
  const viskores::cont::ArrayHandle<viskores::Vec3f_64, viskores::cont::StorageTagBasic>&,
  GradientOutputFields<viskores::Vec3f_64>&);
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template viskores::cont::ArrayHandle<viskores::Vec<viskores::Vec3f_32, 3>>
viskores::worklet::CellGradient::Run(
  const viskores::cont::UnknownCellSet&,
  const viskores::cont::CoordinateSystem&,
  const viskores::cont::ArrayHandle<viskores::Vec3f_32, viskores::cont::StorageTagSOA>&,
  GradientOutputFields<viskores::Vec3f_32>&);
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template viskores::cont::ArrayHandle<viskores::Vec<viskores::Vec3f_64, 3>>
viskores::worklet::CellGradient::Run(
  const viskores::cont::UnknownCellSet&,
  const viskores::cont::CoordinateSystem&,
  const viskores::cont::ArrayHandle<viskores::Vec3f_64, viskores::cont::StorageTagSOA>&,
  GradientOutputFields<viskores::Vec3f_64>&);
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template viskores::cont::ArrayHandle<viskores::Vec<viskores::Vec3f_32, 3>>
viskores::worklet::CellGradient::Run(
  const viskores::cont::UnknownCellSet&,
  const viskores::cont::CoordinateSystem&,
  const viskores::cont::ArrayHandle<
    viskores::Vec3f_32,
    viskores::cont::StorageTagCartesianProduct<viskores::cont::StorageTagBasic,
                                               viskores::cont::StorageTagBasic,
                                               viskores::cont::StorageTagBasic>>&,
  GradientOutputFields<viskores::Vec3f_32>&);
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template viskores::cont::ArrayHandle<viskores::Vec<viskores::Vec3f_64, 3>>
viskores::worklet::CellGradient::Run(
  const viskores::cont::UnknownCellSet&,
  const viskores::cont::CoordinateSystem&,
  const viskores::cont::ArrayHandle<
    viskores::Vec3f_64,
    viskores::cont::StorageTagCartesianProduct<viskores::cont::StorageTagBasic,
                                               viskores::cont::StorageTagBasic,
                                               viskores::cont::StorageTagBasic>>&,
  GradientOutputFields<viskores::Vec3f_64>&);
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template viskores::cont::ArrayHandle<viskores::Vec<viskores::Vec3f, 3>>
viskores::worklet::CellGradient::Run(
  const viskores::cont::UnknownCellSet&,
  const viskores::cont::CoordinateSystem&,
  const viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagUniformPoints>&,
  GradientOutputFields<viskores::Vec3f>&);
VISKORES_INSTANTIATION_END

#endif
