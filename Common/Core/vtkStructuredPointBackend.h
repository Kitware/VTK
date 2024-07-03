// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkStructuredPointBackend
 * @brief A backend for the `vtkImplicitArray` to query structured points efficiently.
 *
 * A backend for the `vtkImplicitArray` to query structured points efficiently.
 *
 * @sa
 * vtkImplicitArray vtkStructuredPointArray
 */

#ifndef vtkStructuredPointBackend_h
#define vtkStructuredPointBackend_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArrayRange.h"   // For vtkDataArrayRange
#include "vtkSmartPointer.h"     // For vtkSmartPointer

#include <array> // For std::array

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
template <typename ValueType>
class VTKCOMMONCORE_EXPORT vtkStructuredPointBackend
{
public:
  //------------------------------------------------------------------------------
  vtkStructuredPointBackend();

  //------------------------------------------------------------------------------
  virtual ~vtkStructuredPointBackend();

  /**
   * These function should only be used when direction matrix is NOT identity.
   */
  virtual ValueType mapStructuredXComponent(int i) const = 0;

  /**
   * These function should only be used when direction matrix is NOT identity.
   */
  virtual ValueType mapStructuredYComponent(int j) const = 0;

  /**
   * These function should only be used when direction matrix is NOT identity.
   */
  virtual ValueType mapStructuredZComponent(int k) const = 0;

  //------------------------------------------------------------------------------
  virtual void mapStructuredTuple(int ijk[3], ValueType tuple[3]) const = 0;

  //------------------------------------------------------------------------------
  virtual void mapTuple(vtkIdType tupleId, ValueType tuple[3]) const = 0;

  //------------------------------------------------------------------------------
  virtual ValueType mapComponent(vtkIdType tupleId, int comp) const = 0;

  //------------------------------------------------------------------------------
  virtual ValueType map(vtkIdType valueId) const = 0;
};

//------------------------------------------------------------------------------
template <typename ValueType, typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ,
  int DataDescription, bool UseDirMatrix>
class vtkStructuredTPointBackend : public vtkStructuredPointBackend<ValueType>
{
  vtkSmartPointer<ArrayTypeX> ArrayX;
  const vtk::detail::ValueRange<ArrayTypeX, 1> X;
  vtkSmartPointer<ArrayTypeY> ArrayY;
  const vtk::detail::ValueRange<ArrayTypeY, 1> Y;
  vtkSmartPointer<ArrayTypeZ> ArrayZ;
  const vtk::detail::ValueRange<ArrayTypeZ, 1> Z;
  const std::array<int, 6> Extent;
  const std::array<vtkIdType, 3> Dimensions;
  const vtkIdType Dimension_0_BY_1;

  // these are used only by vtkImageData and when direction matrix is not identity.
  std::array<double, 16> IndexToPhysicalMatrix;
  // this is a copy of vtkImageData::TransformCoordinates
  void TransformIndexToPhysicalPoint(int i, int j, int k, ValueType* out) const;

public:
  //------------------------------------------------------------------------------
  vtkStructuredTPointBackend();

  //------------------------------------------------------------------------------
  vtkStructuredTPointBackend(
    ArrayTypeX* arrayX, ArrayTypeY* arrayY, ArrayTypeZ* arrayZ, int extent[6], double dirMatrix[9])
    : ArrayX(arrayX)
    , X(vtk::DataArrayValueRange<1>(ArrayX))
    , ArrayY(arrayY)
    , Y(vtk::DataArrayValueRange<1>(ArrayY))
    , ArrayZ(arrayZ)
    , Z(vtk::DataArrayValueRange<1>(ArrayZ))
    , Extent({ { extent[0], extent[1], extent[2], extent[3], extent[4], extent[5] } })
    , Dimensions(
        { { extent[1] - extent[0] + 1, extent[3] - extent[2] + 1, extent[5] - extent[4] + 1 } })
    , Dimension_0_BY_1(this->Dimensions[0] * this->Dimensions[1])
    , IndexToPhysicalMatrix({ {} })
  {
    if (UseDirMatrix)
    {
      assert(this->ArrayX->GetNumberOfTuples() == 2);
      assert(this->ArrayY->GetNumberOfTuples() == 2);
      assert(this->ArrayZ->GetNumberOfTuples() == 2);
      // compute origin
      std::array<double, 3> origin = { { this->X[0], this->Y[0], this->Z[0] } };
      // compute spacing
      std::array<double, 3> spacing = { { this->X[1] - this->X[0], this->Y[1] - this->Y[0],
        this->Z[1] - this->Z[0] } };
      // compute index to physical matrix
      this->IndexToPhysicalMatrix[0] = dirMatrix[0] * spacing[0];
      this->IndexToPhysicalMatrix[1] = dirMatrix[1] * spacing[1];
      this->IndexToPhysicalMatrix[2] = dirMatrix[2] * spacing[2];
      this->IndexToPhysicalMatrix[3] = origin[0];
      this->IndexToPhysicalMatrix[4] = dirMatrix[3] * spacing[0];
      this->IndexToPhysicalMatrix[5] = dirMatrix[4] * spacing[1];
      this->IndexToPhysicalMatrix[6] = dirMatrix[5] * spacing[2];
      this->IndexToPhysicalMatrix[7] = origin[1];
      this->IndexToPhysicalMatrix[8] = dirMatrix[6] * spacing[0];
      this->IndexToPhysicalMatrix[9] = dirMatrix[7] * spacing[1];
      this->IndexToPhysicalMatrix[10] = dirMatrix[8] * spacing[2];
      this->IndexToPhysicalMatrix[11] = origin[2];
      this->IndexToPhysicalMatrix[12] = 0.0;
      this->IndexToPhysicalMatrix[13] = 0.0;
      this->IndexToPhysicalMatrix[14] = 0.0;
      this->IndexToPhysicalMatrix[15] = 1.0;
    }
  }

  //------------------------------------------------------------------------------
  ~vtkStructuredTPointBackend() override;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == 9 /*VTK_EMPTY*/), ValueType>::type VTK_ALWAYS_INLINE
  mapStructuredXComponentImpl(int i) const;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description != 9 /*VTK_EMPTY*/), ValueType>::type VTK_ALWAYS_INLINE
  mapStructuredXComponentImpl(int i) const;

  //------------------------------------------------------------------------------
  ValueType mapStructuredXComponent(int i) const override;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == 9 /*VTK_EMPTY*/), ValueType>::type VTK_ALWAYS_INLINE
  mapStructuredYComponentImpl(int j) const;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description != 9 /*VTK_EMPTY*/), ValueType>::type VTK_ALWAYS_INLINE
  mapStructuredYComponentImpl(int j) const;

  //------------------------------------------------------------------------------
  ValueType mapStructuredYComponent(int j) const override;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == 9 /*VTK_EMPTY*/), ValueType>::type VTK_ALWAYS_INLINE
  mapStructuredZComponentImpl(int k) const;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description != 9 /*VTK_EMPTY*/), ValueType>::type VTK_ALWAYS_INLINE
  mapStructuredZComponentImpl(int k) const;

  //------------------------------------------------------------------------------
  ValueType mapStructuredZComponent(int k) const override;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == 9 /*VTK_EMPTY*/), void>::type VTK_ALWAYS_INLINE
  mapStructuredTupleImpl(int ijk[3], ValueType tuple[3]) const;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description != 9 /*VTK_EMPTY*/), void>::type VTK_ALWAYS_INLINE
  mapStructuredTupleImpl(int ijk[3], ValueType tuple[3]) const;

  //------------------------------------------------------------------------------
  void mapStructuredTuple(int ijk[3], ValueType tuple[3]) const override;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == 9 /*VTK_EMPTY*/), void>::type VTK_ALWAYS_INLINE
  ComputePointStructuredCoords(vtkIdType pointId, int ijk[3]) const;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == 1 /*VTK_SINGLE_POINT*/), void>::type VTK_ALWAYS_INLINE
  ComputePointStructuredCoords(vtkIdType pointId, int ijk[3]) const;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == 2 /*VTK_X_LINE*/), void>::type VTK_ALWAYS_INLINE
  ComputePointStructuredCoords(vtkIdType pointId, int ijk[3]) const;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == 3 /*VTK_Y_LINE*/), void>::type VTK_ALWAYS_INLINE
  ComputePointStructuredCoords(vtkIdType pointId, int ijk[3]) const;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == 4 /*VTK_Z_LINE*/), void>::type VTK_ALWAYS_INLINE
  ComputePointStructuredCoords(vtkIdType pointId, int ijk[3]) const;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == 5 /*VTK_XY_PLANE*/), void>::type VTK_ALWAYS_INLINE
  ComputePointStructuredCoords(vtkIdType pointId, int ijk[3]) const;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == 6 /*VTK_YZ_PLANE*/), void>::type VTK_ALWAYS_INLINE
  ComputePointStructuredCoords(vtkIdType pointId, int ijk[3]) const;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == 7 /*VTK_XZ_PLANE*/), void>::type VTK_ALWAYS_INLINE
  ComputePointStructuredCoords(vtkIdType pointId, int ijk[3]) const;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == 8 /*VTK_XYZ_GRID*/), void>::type VTK_ALWAYS_INLINE
  ComputePointStructuredCoords(vtkIdType pointId, int ijk[3]) const;

  //------------------------------------------------------------------------------
  void mapTuple(vtkIdType tupleId, ValueType tuple[3]) const override;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == 9 /*VTK_EMPTY*/), ValueType>::type VTK_ALWAYS_INLINE
  mapComponentImpl(vtkIdType tupleId, int comp) const;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == 1 /*VTK_SINGLE_POINT*/), ValueType>::type
    VTK_ALWAYS_INLINE
    mapComponentImpl(vtkIdType tupleId, int comp) const;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == 2 /*VTK_X_LINE*/), ValueType>::type VTK_ALWAYS_INLINE
  mapComponentImpl(vtkIdType tupleId, int comp) const;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == 3 /*VTK_Y_LINE*/), ValueType>::type VTK_ALWAYS_INLINE
  mapComponentImpl(vtkIdType tupleId, int comp) const;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == 4 /*VTK_Z_LINE*/), ValueType>::type VTK_ALWAYS_INLINE
  mapComponentImpl(vtkIdType tupleId, int comp) const;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == 5 /*VTK_XY_PLANE*/), ValueType>::type VTK_ALWAYS_INLINE
  mapComponentImpl(vtkIdType tupleId, int comp) const;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == 6 /*VTK_YZ_PLANE*/), ValueType>::type VTK_ALWAYS_INLINE
  mapComponentImpl(vtkIdType tupleId, int comp) const;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == 7 /*VTK_XZ_PLANE*/), ValueType>::type VTK_ALWAYS_INLINE
  mapComponentImpl(vtkIdType tupleId, int comp) const;

  //------------------------------------------------------------------------------
  template <int Description = DataDescription>
  typename std::enable_if<(Description == 8 /*VTK_XYZ_GRID*/), ValueType>::type VTK_ALWAYS_INLINE
  mapComponentImpl(vtkIdType tupleId, int comp) const;

  //------------------------------------------------------------------------------
  ValueType mapComponent(vtkIdType tupleId, int comp) const override;

  //------------------------------------------------------------------------------
  ValueType map(vtkIdType valueId) const override;
};
VTK_ABI_NAMESPACE_END

#endif // vtkStructuredPointBackend_h
/* VTK-HeaderTest-Exclude: vtkStructuredPointBackend.h */

#if defined(VTK_STRUCTURED_POINT_BACKEND_INSTANTIATING)

#define VTK_INSTANTIATE_STRUCTURED_POINT_BACKEND(ValueType)                                        \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template class VTKCOMMONCORE_EXPORT vtkStructuredPointBackend<ValueType>;                        \
  VTK_ABI_NAMESPACE_END
#elif defined(VTK_USE_EXTERN_TEMPLATE)

#ifndef VTK_STRUCTURED_POINT_BACKEND_TEMPLATE_EXTERN
#define VTK_STRUCTURED_POINT_BACKEND_TEMPLATE_EXTERN
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4910) // extern and dllexport incompatible
#endif
VTK_ABI_NAMESPACE_BEGIN
vtkExternTemplateMacro(extern template class VTKCOMMONCORE_EXPORT vtkStructuredPointBackend);
VTK_ABI_NAMESPACE_END
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif // VTK_STRUCTURED_POINT_BACKEND_TEMPLATE_EXTERN

#endif
