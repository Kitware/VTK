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

#ifndef viskores_worklet_gradient_GradientOutput_h
#define viskores_worklet_gradient_GradientOutput_h

#include <viskores/VecTraits.h>

#include <viskores/cont/arg/TransportTagArrayOut.h>
#include <viskores/cont/arg/TransportTagExecObject.h>

#include <viskores/cont/ExecutionObjectBase.h>
#include <viskores/exec/arg/FetchTagArrayDirectOut.h>

#include <viskores/filter/vector_analysis/worklet/gradient/Divergence.h>
#include <viskores/filter/vector_analysis/worklet/gradient/QCriterion.h>
#include <viskores/filter/vector_analysis/worklet/gradient/Vorticity.h>

namespace viskores
{
namespace exec
{

template <typename T>
struct GradientScalarOutputExecutionObject
{
  using ValueType = viskores::Vec<T, 3>;
  using BaseTType = typename viskores::VecTraits<T>::BaseComponentType;

  using HandleType = viskores::cont::ArrayHandle<ValueType>;
  using PortalType = typename HandleType::WritePortalType;

  GradientScalarOutputExecutionObject() = default;

  GradientScalarOutputExecutionObject(viskores::cont::ArrayHandle<ValueType> gradient,
                                      viskores::Id size,
                                      viskores::cont::DeviceAdapterId device,
                                      viskores::cont::Token& token)
  {
    this->GradientPortal = gradient.PrepareForOutput(size, device, token);
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  void Set(viskores::Id index, const viskores::Vec<T, 3>& value) const
  {
    this->GradientPortal.Set(index, value);
  }

  PortalType GradientPortal;
};

template <typename T>
struct GradientScalarOutput : public viskores::cont::ExecutionObjectBase
{
  using ValueType = viskores::Vec<T, 3>;
  using BaseTType = typename viskores::VecTraits<T>::BaseComponentType;

  VISKORES_CONT viskores::exec::GradientScalarOutputExecutionObject<T> PrepareForExecution(
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token) const
  {
    return viskores::exec::GradientScalarOutputExecutionObject<T>(
      this->Gradient, this->Size, device, token);
  }

  GradientScalarOutput(bool,
                       bool,
                       bool,
                       bool,
                       viskores::cont::ArrayHandle<ValueType>& gradient,
                       viskores::cont::ArrayHandle<BaseTType>&,
                       viskores::cont::ArrayHandle<viskores::Vec<BaseTType, 3>>&,
                       viskores::cont::ArrayHandle<BaseTType>&,
                       viskores::Id size)
    : Size(size)
    , Gradient(gradient)
  {
  }
  viskores::Id Size;
  viskores::cont::ArrayHandle<ValueType> Gradient;
};

template <typename T>
struct GradientVecOutputExecutionObject
{
  using ValueType = viskores::Vec<T, 3>;
  using BaseTType = typename viskores::VecTraits<T>::BaseComponentType;

  template <typename FieldType>
  using PortalType = typename viskores::cont::ArrayHandle<FieldType>::WritePortalType;

  GradientVecOutputExecutionObject() = default;

  GradientVecOutputExecutionObject(
    bool g,
    bool d,
    bool v,
    bool q,
    viskores::cont::ArrayHandle<ValueType> gradient,
    viskores::cont::ArrayHandle<BaseTType> divergence,
    viskores::cont::ArrayHandle<viskores::Vec<BaseTType, 3>> vorticity,
    viskores::cont::ArrayHandle<BaseTType> qcriterion,
    viskores::Id size,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    this->SetGradient = g;
    this->SetDivergence = d;
    this->SetVorticity = v;
    this->SetQCriterion = q;

    if (g)
    {
      this->GradientPortal = gradient.PrepareForOutput(size, device, token);
    }
    if (d)
    {
      this->DivergencePortal = divergence.PrepareForOutput(size, device, token);
    }
    if (v)
    {
      this->VorticityPortal = vorticity.PrepareForOutput(size, device, token);
    }
    if (q)
    {
      this->QCriterionPortal = qcriterion.PrepareForOutput(size, device, token);
    }
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  void Set(viskores::Id index, const viskores::Vec<T, 3>& value) const
  {
    if (this->SetGradient)
    {
      this->GradientPortal.Set(index, value);
    }
    if (this->SetDivergence)
    {
      viskores::worklet::gradient::Divergence divergence;
      BaseTType output;
      divergence(value, output);
      this->DivergencePortal.Set(index, output);
    }
    if (this->SetVorticity)
    {
      viskores::worklet::gradient::Vorticity vorticity;
      T output;
      vorticity(value, output);
      this->VorticityPortal.Set(index, output);
    }
    if (this->SetQCriterion)
    {
      viskores::worklet::gradient::QCriterion qc;
      BaseTType output;
      qc(value, output);
      this->QCriterionPortal.Set(index, output);
    }
  }

  bool SetGradient;
  bool SetDivergence;
  bool SetVorticity;
  bool SetQCriterion;

  PortalType<ValueType> GradientPortal;
  PortalType<BaseTType> DivergencePortal;
  PortalType<viskores::Vec<BaseTType, 3>> VorticityPortal;
  PortalType<BaseTType> QCriterionPortal;
};

template <typename T>
struct GradientVecOutput : public viskores::cont::ExecutionObjectBase
{
  using ValueType = viskores::Vec<T, 3>;
  using BaseTType = typename viskores::VecTraits<T>::BaseComponentType;

  VISKORES_CONT viskores::exec::GradientVecOutputExecutionObject<T> PrepareForExecution(
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token) const
  {
    return viskores::exec::GradientVecOutputExecutionObject<T>(this->G,
                                                               this->D,
                                                               this->V,
                                                               this->Q,
                                                               this->Gradient,
                                                               this->Divergence,
                                                               this->Vorticity,
                                                               this->Qcriterion,
                                                               this->Size,
                                                               device,
                                                               token);
  }

  GradientVecOutput() = default;

  GradientVecOutput(bool g,
                    bool d,
                    bool v,
                    bool q,
                    viskores::cont::ArrayHandle<ValueType>& gradient,
                    viskores::cont::ArrayHandle<BaseTType>& divergence,
                    viskores::cont::ArrayHandle<viskores::Vec<BaseTType, 3>>& vorticity,
                    viskores::cont::ArrayHandle<BaseTType>& qcriterion,
                    viskores::Id size)
  {
    this->G = g;
    this->D = d;
    this->V = v;
    this->Q = q;
    this->Gradient = gradient;
    this->Divergence = divergence;
    this->Vorticity = vorticity;
    this->Qcriterion = qcriterion;
    this->Size = size;
  }

  bool G;
  bool D;
  bool V;
  bool Q;
  viskores::cont::ArrayHandle<ValueType> Gradient;
  viskores::cont::ArrayHandle<BaseTType> Divergence;
  viskores::cont::ArrayHandle<viskores::Vec<BaseTType, 3>> Vorticity;
  viskores::cont::ArrayHandle<BaseTType> Qcriterion;
  viskores::Id Size;
};

template <typename T>
struct GradientOutput : public GradientScalarOutput<T>
{
  using GradientScalarOutput<T>::GradientScalarOutput;
};

template <>
struct GradientOutput<viskores::Vec3f_32> : public GradientVecOutput<viskores::Vec3f_32>
{
  using GradientVecOutput<viskores::Vec3f_32>::GradientVecOutput;
};

template <>
struct GradientOutput<viskores::Vec3f_64> : public GradientVecOutput<viskores::Vec3f_64>
{
  using GradientVecOutput<viskores::Vec3f_64>::GradientVecOutput;
};
}
} // namespace viskores::exec


namespace viskores
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for output arrays.
///
/// \c TransportTagArrayOut is a tag used with the \c Transport class to
/// transport \c ArrayHandle objects for output data.
///
struct TransportTagGradientOut
{
};

template <typename ContObjectType, typename Device>
struct Transport<viskores::cont::arg::TransportTagGradientOut, ContObjectType, Device>
{
  using ExecObjectFactoryType = viskores::exec::GradientOutput<typename ContObjectType::ValueType>;
  using ExecObjectType = decltype(std::declval<ExecObjectFactoryType>().PrepareForExecution(
    Device(),
    std::declval<viskores::cont::Token&>()));

  template <typename InputDomainType>
  VISKORES_CONT ExecObjectType operator()(ContObjectType object,
                                          const InputDomainType& viskoresNotUsed(inputDomain),
                                          viskores::Id viskoresNotUsed(inputRange),
                                          viskores::Id outputRange,
                                          viskores::cont::Token& token) const
  {
    ExecObjectFactoryType ExecutionObjectFactory = object.PrepareForOutput(outputRange);
    return ExecutionObjectFactory.PrepareForExecution(Device(), token);
  }
};
}
}
} // namespace viskores::cont::arg


namespace viskores
{
namespace worklet
{
namespace gradient
{


struct GradientOutputs : viskores::cont::arg::ControlSignatureTagBase
{
  using TypeCheckTag = viskores::cont::arg::TypeCheckTagExecObject;
  using TransportTag = viskores::cont::arg::TransportTagGradientOut;
  using FetchTag = viskores::exec::arg::FetchTagArrayDirectOut;
};
}
}
} // namespace viskores::worklet::gradient

#endif
