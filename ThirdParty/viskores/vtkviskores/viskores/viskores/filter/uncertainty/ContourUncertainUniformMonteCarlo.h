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

#ifndef viskores_filter_uncertainty_ContourUncertainUniformMonteCarlo_h
#define viskores_filter_uncertainty_ContourUncertainUniformMonteCarlo_h
#include <viskores/filter/Filter.h>
#include <viskores/filter/uncertainty/viskores_filter_uncertainty_export.h>

namespace viskores
{
namespace filter
{
namespace uncertainty
{
/// \brief Visualize isosurface uncertainty using Monte Carlo approach for uniformly distributed data.
///
/// This filter is implemented to validate the correctness of the ContourUncertainUniform filter.
/// We encourage usage of the ContourUncertainUniform filter because the Monte Carlo approach implemented
/// in this filter is computationally inefficient.
///
class VISKORES_FILTER_UNCERTAINTY_EXPORT ContourUncertainUniformMonteCarlo
  : public viskores::filter::Filter
{
  std::string NumberNonzeroProbabilityName = "num_nonzero_probability";
  std::string EntropyName = "entropy";
  viskores::Float64 IsoValue = 0.0;
  viskores::IdComponent IterValue = 1;

public:
  VISKORES_CONT ContourUncertainUniformMonteCarlo();

  VISKORES_CONT void SetMinField(const std::string& fieldName)
  {
    this->SetActiveField(0, fieldName, viskores::cont::Field::Association::Points);
  }
  VISKORES_CONT void SetMaxField(const std::string& fieldName)
  {
    this->SetActiveField(1, fieldName, viskores::cont::Field::Association::Points);
  }
  VISKORES_CONT void SetIsoValue(viskores::Float64 value) { this->IsoValue = value; }
  VISKORES_CONT viskores::Float64 GetIsoValue() const { return this->IsoValue; }

  VISKORES_CONT void SetNumSample(viskores::IdComponent value) { this->IterValue = value; }
  VISKORES_CONT viskores::IdComponent GetNumSample() const { return this->IterValue; }

  VISKORES_CONT void SetCrossProbabilityName(const std::string& name)
  {
    this->SetOutputFieldName(name);
  }
  VISKORES_CONT const std::string& GetCrossProbabilityName() const
  {
    return this->GetOutputFieldName();
  }

  VISKORES_CONT void SetNumberNonzeroProbabilityName(const std::string& name)
  {
    this->NumberNonzeroProbabilityName = name;
  }
  VISKORES_CONT const std::string& GetNumberNonzeroProbabilityName() const
  {
    return this->NumberNonzeroProbabilityName;
  }
  VISKORES_CONT void SetEntropyName(const std::string& name) { this->EntropyName = name; }
  VISKORES_CONT const std::string& GetEntropyName() const { return this->EntropyName; }

protected:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
};
}
}
}
#endif
