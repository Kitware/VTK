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

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleGroupVecVariable.h>
#include <viskores/cont/ArrayHandleRandomUniformReal.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Timer.h>
#include <viskores/filter/uncertainty/ContourUncertainUniformMonteCarlo.h>
#include <viskores/worklet/WorkletMapTopology.h>

namespace viskores
{
namespace worklet
{
namespace uniform
{
class ContourUncertainUniformMonteCarlo : public viskores::worklet::WorkletVisitCellsWithPoints
{

public:
  ContourUncertainUniformMonteCarlo(double isovalue, int itervalue)
    : m_isovalue(isovalue)
    , m_numsample(itervalue)
  {
  }
  using ControlSignature = void(CellSetIn,
                                FieldInPoint,
                                FieldInPoint,
                                FieldInCell,
                                FieldOutCell,
                                FieldOutCell,
                                FieldOutCell);
  using ExecutionSignature = void(_2, _3, _4, _5, _6, _7);
  using InputDomain = _1;

  template <typename InPointFieldMinType,
            typename InPointFieldMaxType,
            typename InRandomNumbersType,
            typename OutCellFieldType1,
            typename OutCellFieldType2,
            typename OutCellFieldType3>

  VISKORES_EXEC void operator()(const InPointFieldMinType& inPointFieldVecMin,
                                const InPointFieldMaxType& inPointFieldVecMax,
                                const InRandomNumbersType& randomNumbers,
                                OutCellFieldType1& outNonCrossProb,
                                OutCellFieldType2& outCrossProb,
                                OutCellFieldType3& outEntropyProb) const
  {
    viskores::IdComponent numPoints = inPointFieldVecMin.GetNumberOfComponents();

    if (numPoints != 8)
    {
      this->RaiseError("This is the 3D version for 8 vertices\n");
      return;
    }

    viskores::FloatDefault minV = 0.0;
    viskores::FloatDefault maxV = 0.0;
    viskores::FloatDefault uniformDistValue = 0.0;
    viskores::IdComponent numSample = this->m_numsample;
    viskores::FloatDefault numCrossing = 0;
    viskores::FloatDefault crossProb = 0;

    viskores::IdComponent zeroFlag;
    viskores::IdComponent oneFlag;
    viskores::Float64 base = 2.0;
    viskores::Float64 totalSum = 0.0;
    viskores::IdComponent nonZeroCase = 0;
    viskores::FloatDefault entropyValue = 0;
    viskores::FloatDefault templog = 0;
    viskores::FloatDefault value = 0.0;
    viskores::IdComponent k = 0;

    constexpr viskores::IdComponent totalNumCases = 256;
    viskores::Vec<viskores::FloatDefault, totalNumCases> probHistogram;

    for (viskores::IdComponent j = 0; j < totalNumCases; j++)
    {
      probHistogram[j] = 0;
    }

    for (viskores::IdComponent i = 0; i < numSample; ++i)
    {
      zeroFlag = 0;
      oneFlag = 0;
      totalSum = 0.0;
      for (viskores::IdComponent pointIndex = 0; pointIndex < numPoints; ++pointIndex)
      {
        minV = static_cast<viskores::FloatDefault>(inPointFieldVecMin[pointIndex]);
        maxV = static_cast<viskores::FloatDefault>(inPointFieldVecMax[pointIndex]);

        auto tmp = randomNumbers[k];

        uniformDistValue = minV + tmp * (maxV - minV);

        if (uniformDistValue <= this->m_isovalue)
        {
          zeroFlag = 1;
        }
        else
        {
          oneFlag = 1;
          totalSum += viskores::Pow(base, pointIndex);
        }

        k += 1;
      }

      if ((oneFlag == 1) && (zeroFlag == 1))
      {
        numCrossing += 1;
      }

      if ((totalSum >= 0) && (totalSum <= 255))
      {
        probHistogram[static_cast<viskores::IdComponent>(totalSum)] += 1;
      }
    }

    for (viskores::IdComponent i = 0; i < totalNumCases; i++)
    {
      templog = 0;
      value = static_cast<viskores::FloatDefault>(probHistogram[i] /
                                                  static_cast<viskores::FloatDefault>(numSample));
      if (probHistogram[i] > 0.00001)
      {
        nonZeroCase++;
        templog = viskores::Log2(value);
      }
      entropyValue = entropyValue + (-value) * templog;
    }

    crossProb = numCrossing / static_cast<viskores::FloatDefault>(numSample);
    outNonCrossProb = static_cast<viskores::FloatDefault>(nonZeroCase);
    outCrossProb = crossProb;
    outEntropyProb = entropyValue;
  }

private:
  double m_isovalue;
  int m_numsample;
};
}
}
}

namespace viskores
{
namespace filter
{
namespace uncertainty
{
ContourUncertainUniformMonteCarlo::ContourUncertainUniformMonteCarlo()
{
  this->SetCrossProbabilityName("cross_probability");
}
VISKORES_CONT viskores::cont::DataSet ContourUncertainUniformMonteCarlo::DoExecute(
  const viskores::cont::DataSet& input)
{
  viskores::cont::Field minField = this->GetFieldFromDataSet(0, input);
  viskores::cont::Field maxField = this->GetFieldFromDataSet(1, input);

  viskores::cont::UnknownArrayHandle crossProbability;
  viskores::cont::UnknownArrayHandle nonCrossProbability;
  viskores::cont::UnknownArrayHandle entropyProbability;

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
    viskores::cont::ArrayHandle<ValueType> concreteNonCrossProb;
    viskores::cont::ArrayHandle<ValueType> concreteEntropyProb;

    viskores::cont::ArrayHandleRandomUniformReal<viskores::FloatDefault> randomArray(
      cellSet.GetNumberOfCells() * this->IterValue * 8, { 0xceed });

    this->Invoke(viskores::worklet::uniform::ContourUncertainUniformMonteCarlo{ this->IsoValue,
                                                                                this->IterValue },
                 cellSet,
                 concreteMinField,
                 concreteMaxField,
                 viskores::cont::make_ArrayHandleGroupVecVariable(
                   randomArray,
                   viskores::cont::ArrayHandleCounting<viskores::Id>(
                     0, this->IterValue * 8, cellSet.GetNumberOfCells() + 1)),
                 concreteNonCrossProb,
                 concreteCrossProb,
                 concreteEntropyProb);

    crossProbability = concreteCrossProb;
    nonCrossProbability = concreteNonCrossProb;
    entropyProbability = concreteEntropyProb;
  };
  this->CastAndCallScalarField(minField, resolveType);
  viskores::cont::DataSet result = this->CreateResult(input);
  result.AddCellField(this->GetCrossProbabilityName(), crossProbability);
  result.AddCellField(this->GetNumberNonzeroProbabilityName(), nonCrossProbability);
  result.AddCellField(this->GetEntropyName(), entropyProbability);
  return result;
}
}
}
}
