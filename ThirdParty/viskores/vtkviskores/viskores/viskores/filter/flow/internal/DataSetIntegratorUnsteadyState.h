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

#ifndef viskores_filter_flow_internal_DataSetIntegratorUnsteadyState_h
#define viskores_filter_flow_internal_DataSetIntegratorUnsteadyState_h

#include <viskores/cont/ArrayCopy.h>
#include <viskores/filter/flow/internal/DataSetIntegrator.h>
#include <viskores/filter/flow/worklet/TemporalGridEvaluators.h>

namespace viskores
{
namespace filter
{
namespace flow
{
namespace internal
{
namespace detail
{
template <typename ParticleType,
          typename FieldType,
          typename TerminationType,
          typename AnalysisType>
class AdvectHelperUnsteadyState
{
public:
  using WorkletType = viskores::worklet::flow::ParticleAdvection;
  using UnsteadyStateGridEvalType = viskores::worklet::flow::TemporalGridEvaluator<FieldType>;

  template <template <typename> class SolverType>
  static void DoAdvect(viskores::cont::ArrayHandle<ParticleType>& seedArray,
                       const FieldType& field1,
                       const viskores::cont::DataSet& ds1,
                       viskores::FloatDefault t1,
                       const FieldType& field2,
                       const viskores::cont::DataSet& ds2,
                       viskores::FloatDefault t2,
                       const TerminationType& termination,
                       viskores::FloatDefault stepSize,
                       AnalysisType& analysis)

  {
    using StepperType = viskores::worklet::flow::Stepper<SolverType<UnsteadyStateGridEvalType>,
                                                         UnsteadyStateGridEvalType>;
    WorkletType worklet;
    UnsteadyStateGridEvalType eval(ds1, t1, field1, ds2, t2, field2);
    StepperType stepper(eval, stepSize);
    worklet.Run(stepper, seedArray, termination, analysis);
  }

  static void Advect(viskores::cont::ArrayHandle<ParticleType>& seedArray,
                     const FieldType& field1,
                     const viskores::cont::DataSet& ds1,
                     viskores::FloatDefault t1,
                     const FieldType& field2,
                     const viskores::cont::DataSet& ds2,
                     viskores::FloatDefault t2,
                     const TerminationType& termination,
                     const IntegrationSolverType& solverType,
                     viskores::FloatDefault stepSize,
                     AnalysisType& analysis)
  {
    if (solverType == IntegrationSolverType::RK4_TYPE)
    {
      DoAdvect<viskores::worklet::flow::RK4Integrator>(
        seedArray, field1, ds1, t1, field2, ds2, t2, termination, stepSize, analysis);
    }
    else if (solverType == IntegrationSolverType::EULER_TYPE)
    {
      DoAdvect<viskores::worklet::flow::EulerIntegrator>(
        seedArray, field1, ds1, t1, field2, ds2, t2, termination, stepSize, analysis);
    }
    else
      throw viskores::cont::ErrorFilterExecution("Unsupported Integrator type");
  }
};
} //namespace detail

template <typename ParticleType,
          typename FieldType,
          typename TerminationType,
          typename AnalysisType>
class DataSetIntegratorUnsteadyState
  : public viskores::filter::flow::internal::DataSetIntegrator<
      DataSetIntegratorUnsteadyState<ParticleType, FieldType, TerminationType, AnalysisType>,
      ParticleType>
{
public:
  using BaseType = viskores::filter::flow::internal::DataSetIntegrator<
    DataSetIntegratorUnsteadyState<ParticleType, FieldType, TerminationType, AnalysisType>,
    ParticleType>;
  using PType = ParticleType;
  using FType = FieldType;
  using TType = TerminationType;
  using AType = AnalysisType;

  DataSetIntegratorUnsteadyState(viskores::Id id,
                                 const FieldType& field1,
                                 const FieldType& field2,
                                 const viskores::cont::DataSet& ds1,
                                 const viskores::cont::DataSet& ds2,
                                 viskores::FloatDefault t1,
                                 viskores::FloatDefault t2,
                                 viskores::filter::flow::IntegrationSolverType solverType,
                                 const TerminationType& termination,
                                 const AnalysisType& analysis)
    : BaseType(id, solverType)
    , Field1(field1)
    , Field2(field2)
    , DataSet1(ds1)
    , DataSet2(ds2)
    , Time1(t1)
    , Time2(t2)
    , Termination(termination)
    , Analysis(analysis)
  {
  }

  VISKORES_CONT inline void DoAdvect(
    viskores::filter::flow::internal::DSIHelperInfo<ParticleType>& block,
    viskores::FloatDefault stepSize)
  {
    auto copyFlag = (this->CopySeedArray ? viskores::CopyFlag::On : viskores::CopyFlag::Off);
    auto seedArray = viskores::cont::make_ArrayHandle(block.Particles, copyFlag);

    using AdvectionHelper =
      detail::AdvectHelperUnsteadyState<ParticleType, FieldType, TerminationType, AnalysisType>;
    AnalysisType analysis;
    analysis.UseAsTemplate(this->Analysis);

    AdvectionHelper::Advect(seedArray,
                            this->Field1,
                            this->DataSet1,
                            this->Time1,
                            this->Field2,
                            this->DataSet2,
                            this->Time2,
                            this->Termination,
                            this->SolverType,
                            stepSize,
                            analysis);
    this->UpdateResult(analysis, block);
  }

  VISKORES_CONT void UpdateResult(
    AnalysisType& analysis,
    viskores::filter::flow::internal::DSIHelperInfo<ParticleType>& dsiInfo)
  {
    this->ClassifyParticles(analysis.Particles, dsiInfo);
    if (std::is_same<AnalysisType, viskores::worklet::flow::NoAnalysis<ParticleType>>::value)
    {
      if (dsiInfo.TermIdx.empty())
        return;
      auto indicesAH = viskores::cont::make_ArrayHandle(dsiInfo.TermIdx, viskores::CopyFlag::Off);
      auto termPerm = viskores::cont::make_ArrayHandlePermutation(indicesAH, analysis.Particles);
      viskores::cont::ArrayHandle<ParticleType> termParticles;
      viskores::cont::Algorithm::Copy(termPerm, termParticles);
      analysis.FinalizeAnalysis(termParticles);
      this->Analyses.emplace_back(analysis);
    }
    else
    {
      this->Analyses.emplace_back(analysis);
    }
  }

  VISKORES_CONT bool GetOutput(viskores::cont::DataSet& ds) const
  {
    std::size_t nAnalyses = this->Analyses.size();
    if (nAnalyses == 0)
      return false;
    return AnalysisType::MakeDataSet(ds, this->Analyses);
  }

private:
  FieldType Field1;
  FieldType Field2;
  viskores::cont::DataSet DataSet1;
  viskores::cont::DataSet DataSet2;
  viskores::FloatDefault Time1;
  viskores::FloatDefault Time2;
  TerminationType Termination;
  AnalysisType Analysis;
  std::vector<AnalysisType> Analyses;
};

}
}
}
} //viskores::filter::flow::internal

#endif //viskores_filter_flow_internal_DataSetIntegratorUnsteadyState_h
