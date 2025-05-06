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

#ifndef viskores_filter_flow_worklet_Field_h
#define viskores_filter_flow_worklet_Field_h

#include <viskores/Types.h>

#include <viskores/VecVariable.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ExecutionObjectBase.h>
#include <viskores/exec/CellInterpolate.h>

namespace viskores
{
namespace worklet
{
namespace flow
{

template <typename FieldArrayType>
class ExecutionVelocityField
{
public:
  using FieldPortalType = typename FieldArrayType::ReadPortalType;
  using Association = viskores::cont::Field::Association;
  using DelegateToField = std::false_type;

  VISKORES_CONT
  ExecutionVelocityField(const FieldArrayType& velocityValues,
                         const Association assoc,
                         viskores::cont::DeviceAdapterId device,
                         viskores::cont::Token& token)
    : VelocityValues(velocityValues.PrepareForInput(device, token))
    , Assoc(assoc)
  {
  }

  VISKORES_EXEC Association GetAssociation() const { return this->Assoc; }

  VISKORES_EXEC void GetValue(const viskores::Id cellId,
                              viskores::VecVariable<viskores::Vec3f, 2>& value) const
  {
    VISKORES_ASSERT(this->Assoc == Association::Cells);

    viskores::Vec3f velocity = VelocityValues.Get(cellId);
    value = viskores::make_Vec(velocity);
  }

  VISKORES_EXEC void GetValue(const viskores::VecVariable<viskores::Id, 8>& indices,
                              const viskores::Id vertices,
                              const viskores::Vec3f& parametric,
                              const viskores::UInt8 cellShape,
                              viskores::VecVariable<viskores::Vec3f, 2>& value) const
  {
    VISKORES_ASSERT(this->Assoc == Association::Points);

    viskores::Vec3f velocityInterp;
    viskores::VecVariable<viskores::Vec3f, 8> velocities;
    for (viskores::IdComponent i = 0; i < vertices; i++)
      velocities.Append(VelocityValues.Get(indices[i]));
    viskores::exec::CellInterpolate(velocities, parametric, cellShape, velocityInterp);
    value = viskores::make_Vec(velocityInterp);
  }

  template <typename Point, typename Locator, typename Helper>
  VISKORES_EXEC bool GetValue(const Point& viskoresNotUsed(point),
                              const viskores::FloatDefault& viskoresNotUsed(time),
                              viskores::VecVariable<Point, 2>& viskoresNotUsed(out),
                              const Locator& viskoresNotUsed(locator),
                              const Helper& viskoresNotUsed(helper)) const
  {
    //TODO Raise Error : Velocity Field should not allow this path
    return false;
  }

private:
  FieldPortalType VelocityValues;
  Association Assoc;
};

template <typename FieldArrayType>
class VelocityField : public viskores::cont::ExecutionObjectBase
{
public:
  using ExecutionType = ExecutionVelocityField<FieldArrayType>;
  using Association = viskores::cont::Field::Association;

  VISKORES_CONT
  VelocityField() = default;

  VISKORES_CONT
  VelocityField(const FieldArrayType& fieldValues)
    : FieldValues(fieldValues)
    , Assoc(viskores::cont::Field::Association::Points)
  {
  }

  VISKORES_CONT
  VelocityField(const FieldArrayType& fieldValues, const Association assoc)
    : FieldValues(fieldValues)
    , Assoc(assoc)
  {
    if (assoc != Association::Points && assoc != Association::Cells)
      throw("Unsupported field association");
  }

  VISKORES_CONT
  const ExecutionType PrepareForExecution(viskores::cont::DeviceAdapterId device,
                                          viskores::cont::Token& token) const
  {
    return ExecutionType(this->FieldValues, this->Assoc, device, token);
  }

private:
  FieldArrayType FieldValues;
  Association Assoc;
};

template <typename FieldArrayType>
class ExecutionElectroMagneticField
{
public:
  using FieldPortalType = typename FieldArrayType::ReadPortalType;
  using Association = viskores::cont::Field::Association;
  using DelegateToField = std::false_type;

  VISKORES_CONT
  ExecutionElectroMagneticField(const FieldArrayType& electricValues,
                                const FieldArrayType& magneticValues,
                                const Association assoc,
                                viskores::cont::DeviceAdapterId device,
                                viskores::cont::Token& token)
    : ElectricValues(electricValues.PrepareForInput(device, token))
    , MagneticValues(magneticValues.PrepareForInput(device, token))
    , Assoc(assoc)
  {
  }

  VISKORES_EXEC Association GetAssociation() const { return this->Assoc; }

  VISKORES_EXEC void GetValue(const viskores::Id cellId,
                              viskores::VecVariable<viskores::Vec3f, 2>& value) const
  {
    VISKORES_ASSERT(this->Assoc == Association::Cells);

    auto electric = this->ElectricValues.Get(cellId);
    auto magnetic = this->MagneticValues.Get(cellId);
    value = viskores::make_Vec(electric, magnetic);
  }

  VISKORES_EXEC void GetValue(const viskores::VecVariable<viskores::Id, 8>& indices,
                              const viskores::Id vertices,
                              const viskores::Vec3f& parametric,
                              const viskores::UInt8 cellShape,
                              viskores::VecVariable<viskores::Vec3f, 2>& value) const
  {
    VISKORES_ASSERT(this->Assoc == Association::Points);

    viskores::Vec3f electricInterp, magneticInterp;
    viskores::VecVariable<viskores::Vec3f, 8> electric;
    viskores::VecVariable<viskores::Vec3f, 8> magnetic;
    for (viskores::IdComponent i = 0; i < vertices; i++)
    {
      electric.Append(ElectricValues.Get(indices[i]));
      magnetic.Append(MagneticValues.Get(indices[i]));
    }
    viskores::exec::CellInterpolate(electric, parametric, cellShape, electricInterp);
    viskores::exec::CellInterpolate(magnetic, parametric, cellShape, magneticInterp);
    value = viskores::make_Vec(electricInterp, magneticInterp);
  }

  template <typename Point, typename Locator, typename Helper>
  VISKORES_EXEC bool GetValue(const Point& viskoresNotUsed(point),
                              const viskores::FloatDefault& viskoresNotUsed(time),
                              viskores::VecVariable<Point, 2>& viskoresNotUsed(out),
                              const Locator& viskoresNotUsed(locator),
                              const Helper& viskoresNotUsed(helper)) const
  {
    //TODO : Raise Error : Velocity Field should not allow this path
    return false;
  }

private:
  FieldPortalType ElectricValues;
  FieldPortalType MagneticValues;
  Association Assoc;
};

template <typename FieldArrayType>
class ElectroMagneticField : public viskores::cont::ExecutionObjectBase
{
public:
  using ExecutionType = ExecutionElectroMagneticField<FieldArrayType>;
  using Association = viskores::cont::Field::Association;

  VISKORES_CONT
  ElectroMagneticField() = default;

  VISKORES_CONT
  ElectroMagneticField(const FieldArrayType& electricField, const FieldArrayType& magneticField)
    : ElectricField(electricField)
    , MagneticField(magneticField)
    , Assoc(viskores::cont::Field::Association::Points)
  {
  }

  VISKORES_CONT
  ElectroMagneticField(const FieldArrayType& electricField,
                       const FieldArrayType& magneticField,
                       const Association assoc)
    : ElectricField(electricField)
    , MagneticField(magneticField)
    , Assoc(assoc)
  {
  }

  VISKORES_CONT
  const ExecutionType PrepareForExecution(viskores::cont::DeviceAdapterId device,
                                          viskores::cont::Token& token) const
  {
    return ExecutionType(this->ElectricField, this->MagneticField, this->Assoc, device, token);
  }

private:
  FieldArrayType ElectricField;
  FieldArrayType MagneticField;
  Association Assoc;
};

}
}
} //viskores::worklet::flow

#endif //viskores_filter_flow_worklet_Field_h
