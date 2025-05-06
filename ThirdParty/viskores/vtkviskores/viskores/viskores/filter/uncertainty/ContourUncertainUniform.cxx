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

//  This code is based on the algorithm presented in the following papers:
//  Wang, J., Athawale, T., Moreland, K., Chen, J., Johnson, C., & Pugmire,
//  D. (2023). FunMC^ 2: A Filter for Uncertainty Visualization of Marching
//  Cubes on Multi-Core Devices. Oak Ridge National Laboratory (ORNL),
//  Oak Ridge, TN (United States).
//
//  Athawale, T. M., Sane, S., & Johnson, C. R. (2021, October). Uncertainty
//  Visualization of the Marching Squares and Marching Cubes Topology Cases.
//  In 2021 IEEE Visualization Conference (VIS) (pp. 106-110). IEEE.

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Timer.h>
#include <viskores/filter/uncertainty/ContourUncertainUniform.h>
#include <viskores/worklet/WorkletMapTopology.h>

namespace
{
class ClosedFormUniform : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  ClosedFormUniform(double isovalue)
    : m_isovalue(isovalue){};
  using ControlSignature =
    void(CellSetIn, FieldInPoint, FieldInPoint, FieldOutCell, FieldOutCell, FieldOutCell);

  using ExecutionSignature = void(_2, _3, _4, _5, _6);

  using InputDomain = _1;
  template <typename InPointFieldMinType,
            typename InPointFieldMaxType,
            typename OutCellFieldType1,
            typename OutCellFieldType2,
            typename OutCellFieldType3>

  VISKORES_EXEC void operator()(const InPointFieldMinType& inPointFieldVecMin,
                                const InPointFieldMaxType& inPointFieldVecMax,
                                OutCellFieldType1& outCellFieldCProb,
                                OutCellFieldType2& outCellFieldNumNonzeroProb,
                                OutCellFieldType3& outCellFieldEntropy) const
  {
    viskores::IdComponent numPoints = inPointFieldVecMin.GetNumberOfComponents();

    if (numPoints != 8)
    {
      this->RaiseError("This is the 3D version for 8 vertices\n");
      return;
    }

    viskores::FloatDefault allPositiveProb = 1.0;
    viskores::FloatDefault allNegativeProb = 1.0;
    viskores::FloatDefault allCrossProb = 0.0;
    viskores::FloatDefault positiveProb;
    viskores::FloatDefault negativeProb;
    viskores::Vec<viskores::Vec2f, 8> ProbList;

    constexpr viskores::IdComponent totalNumCases = 256;
    viskores::Vec<viskores::FloatDefault, totalNumCases> probHistogram;

    for (viskores::IdComponent pointIndex = 0; pointIndex < numPoints; ++pointIndex)
    {
      viskores::FloatDefault minV =
        static_cast<viskores::FloatDefault>(inPointFieldVecMin[pointIndex]);
      viskores::FloatDefault maxV =
        static_cast<viskores::FloatDefault>(inPointFieldVecMax[pointIndex]);

      if (this->m_isovalue <= minV)
      {
        positiveProb = 1.0;
        negativeProb = 0.0;
      }
      else if (this->m_isovalue >= maxV)
      {
        positiveProb = 0.0;
        negativeProb = 1.0;
      }
      else
      {
        positiveProb =
          static_cast<viskores::FloatDefault>((maxV - (this->m_isovalue)) / (maxV - minV));
        negativeProb = 1 - positiveProb;
      }

      allNegativeProb *= negativeProb;
      allPositiveProb *= positiveProb;

      ProbList[pointIndex][0] = negativeProb;
      ProbList[pointIndex][1] = positiveProb;
    }

    allCrossProb = 1 - allPositiveProb - allNegativeProb;
    outCellFieldCProb = allCrossProb;

    TraverseBit(ProbList, probHistogram);

    viskores::FloatDefault entropyValue = 0;
    viskores::Id nonzeroCases = 0;
    viskores::FloatDefault templog = 0;

    for (viskores::IdComponent i = 0; i < totalNumCases; i++)
    {
      templog = 0;
      if (probHistogram[i] > 0.00001)
      {
        nonzeroCases++;
        templog = viskores::Log2(probHistogram[i]);
      }
      entropyValue = entropyValue + (-probHistogram[i]) * templog;
    }

    outCellFieldNumNonzeroProb = nonzeroCases;
    outCellFieldEntropy = entropyValue;
  }

  VISKORES_EXEC inline void TraverseBit(
    viskores::Vec<viskores::Vec2f, 8>& ProbList,
    viskores::Vec<viskores::FloatDefault, 256>& probHistogram) const
  {

    for (viskores::IdComponent i = 0; i < 256; i++)
    {
      viskores::FloatDefault currProb = 1.0;
      for (viskores::IdComponent j = 0; j < 8; j++)
      {
        if (i & (1 << j))
        {
          currProb *= ProbList[j][1];
        }
        else
        {
          currProb *= ProbList[j][0];
        }
      }
      probHistogram[i] = currProb;
    }
  }

private:
  double m_isovalue;
};
}

namespace viskores
{
namespace filter
{
namespace uncertainty
{
ContourUncertainUniform::ContourUncertainUniform()
{
  this->SetCrossProbabilityName("cross_probability");
}
VISKORES_CONT viskores::cont::DataSet ContourUncertainUniform::DoExecute(
  const viskores::cont::DataSet& input)
{
  viskores::cont::Field minField = this->GetFieldFromDataSet(0, input);
  viskores::cont::Field maxField = this->GetFieldFromDataSet(1, input);

  viskores::cont::UnknownArrayHandle crossProbability;
  viskores::cont::UnknownArrayHandle numNonZeroProbability;
  viskores::cont::UnknownArrayHandle entropy;

  if (!input.GetCellSet().IsType<viskores::cont::CellSetStructured<3>>())
  {
    throw viskores::cont::ErrorBadType("Uncertain contour only works for CellSetStructured<3>.");
  }
  viskores::cont::CellSetStructured<3> cellSet;
  input.GetCellSet().AsCellSet(cellSet);

  auto resolveType = [&](auto concreteMinField)
  {
    using ArrayType = std::decay_t<decltype(concreteMinField)>;
    using ValueType = typename ArrayType::ValueType;
    ArrayType concreteMaxField;
    viskores::cont::ArrayCopyShallowIfPossible(maxField.GetData(), concreteMaxField);

    viskores::cont::ArrayHandle<ValueType> concreteCrossProb;
    viskores::cont::ArrayHandle<viskores::Id> concreteNumNonZeroProb;
    viskores::cont::ArrayHandle<ValueType> concreteEntropy;
    this->Invoke(ClosedFormUniform{ this->IsoValue },
                 cellSet,
                 concreteMinField,
                 concreteMaxField,
                 concreteCrossProb,
                 concreteNumNonZeroProb,
                 concreteEntropy);
    crossProbability = concreteCrossProb;
    numNonZeroProbability = concreteNumNonZeroProb;
    entropy = concreteEntropy;
  };
  this->CastAndCallScalarField(minField, resolveType);

  viskores::cont::DataSet result = this->CreateResult(input);
  result.AddCellField(this->GetCrossProbabilityName(), crossProbability);
  result.AddCellField(this->GetNumberNonzeroProbabilityName(), numNonZeroProbability);
  result.AddCellField(this->GetEntropyName(), entropy);
  return result;
}
}
}
}
