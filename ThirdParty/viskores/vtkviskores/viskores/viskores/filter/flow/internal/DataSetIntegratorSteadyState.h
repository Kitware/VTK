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

#ifndef viskores_filter_flow_internal_DataSetIntegratorSteadyState_h
#define viskores_filter_flow_internal_DataSetIntegratorSteadyState_h

#include <viskores/cont/ArrayCopy.h>
#include <viskores/filter/flow/internal/DataSetIntegrator.h>

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
class AdvectHelperSteadyState
{
public:
  using WorkletType = viskores::worklet::flow::ParticleAdvection;
  using SteadyStateGridEvalType = viskores::worklet::flow::GridEvaluator<FieldType>;

  template <template <typename> class SolverType>
  static void DoAdvect(viskores::cont::ArrayHandle<ParticleType>& seedArray,
                       const FieldType& field,
                       const viskores::cont::DataSet& dataset,
                       const TerminationType& termination,
                       viskores::FloatDefault stepSize,
                       AnalysisType& analysis)
  {
    using StepperType = viskores::worklet::flow::Stepper<SolverType<SteadyStateGridEvalType>,
                                                         SteadyStateGridEvalType>;
    SteadyStateGridEvalType eval(dataset, field);
    StepperType stepper(eval, stepSize);

    WorkletType worklet;
    worklet.Run(stepper, seedArray, termination, analysis);
  }

  static void Advect(viskores::cont::ArrayHandle<ParticleType>& seedArray,
                     const FieldType& field,
                     const viskores::cont::DataSet& dataset,
                     const TerminationType& termination,
                     const IntegrationSolverType& solverType,
                     viskores::FloatDefault stepSize,
                     AnalysisType& analysis)
  {
    if (solverType == IntegrationSolverType::RK4_TYPE)
    {
      DoAdvect<viskores::worklet::flow::RK4Integrator>(
        seedArray, field, dataset, termination, stepSize, analysis);
    }
    else if (solverType == IntegrationSolverType::EULER_TYPE)
    {
      DoAdvect<viskores::worklet::flow::EulerIntegrator>(
        seedArray, field, dataset, termination, stepSize, analysis);
    }
    else
      throw viskores::cont::ErrorFilterExecution("Unsupported Integrator type");
  }
};
} // namespace detail

template <typename ParticleType,
          typename FieldType,
          typename TerminationType,
          typename AnalysisType>
class DataSetIntegratorSteadyState
  : public viskores::filter::flow::internal::DataSetIntegrator<
      DataSetIntegratorSteadyState<ParticleType, FieldType, TerminationType, AnalysisType>,
      ParticleType>
{
public:
  using BaseType = viskores::filter::flow::internal::DataSetIntegrator<
    DataSetIntegratorSteadyState<ParticleType, FieldType, TerminationType, AnalysisType>,
    ParticleType>;
  using PType = ParticleType;
  using FType = FieldType;
  using TType = TerminationType;
  using AType = AnalysisType;

  DataSetIntegratorSteadyState(viskores::Id id,
                               const FieldType& field,
                               const viskores::cont::DataSet& dataset,
                               viskores::filter::flow::IntegrationSolverType solverType,
                               const TerminationType& termination,
                               const AnalysisType& analysis)
    : BaseType(id, solverType)
    , Field(field)
    , Dataset(dataset)
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
      detail::AdvectHelperSteadyState<ParticleType, FieldType, TerminationType, AnalysisType>;
    AnalysisType analysis;
    analysis.UseAsTemplate(this->Analysis);

    AdvectionHelper::Advect(seedArray,
                            this->Field,
                            this->Dataset,
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
  FieldType Field;
  viskores::cont::DataSet Dataset;
  TerminationType Termination;
  // Used as a template to initialize successive analysis objects.
  AnalysisType Analysis;
  std::vector<AnalysisType> Analyses;
};

}
}
}
} //viskores::filter::flow::internal

#endif //viskores_filter_flow_internal_DataSetIntegratorSteadyState_h
