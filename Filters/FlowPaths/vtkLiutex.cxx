// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLiutex.h"

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGradientFilter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <cmath>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkLiutex);
VTK_ABI_NAMESPACE_END

namespace
{
struct LiutexValues
{
  double Vector[3] = { 0., 0., 0. };
  double Magnitude = 0.;
  // beta^2 - alpha^2, which equals lambda_ci^2
  double LambdaCi2 = 0.;
  double Beta2 = 0.;
  // beta^2 + alpha^2 + lambda_cr^2 + lambda_r^2 / 2
  double OmegaDenominator = 0.;
};

// j[3 * i + k] = du_i/dx_k
LiutexValues ComputeLiutex(const double j[9])
{
  LiutexValues out;

  // Characteristic polynomial lambda^3 - i1 lambda^2 + i2 lambda - i3, solved
  // with Cardano's formula after the shift lambda = t + i1 / 3.
  const double i1 = j[0] + j[4] + j[8];
  const double i2 =
    j[0] * j[4] - j[1] * j[3] + j[0] * j[8] - j[2] * j[6] + j[4] * j[8] - j[5] * j[7];
  const double i3 = j[0] * (j[4] * j[8] - j[5] * j[7]) - j[1] * (j[3] * j[8] - j[5] * j[6]) +
    j[2] * (j[3] * j[7] - j[4] * j[6]);
  const double shift = i1 / 3.;
  const double p = i2 - i1 * i1 / 3.;
  const double q = -2. * i1 * i1 * i1 / 27. + i1 * i2 / 3. - i3;

  const double halfQ = q / 2.;
  const double thirdP = p / 3.;
  const double discriminant = halfQ * halfQ + thirdP * thirdP * thirdP;
  if (!(discriminant > 0.))
  {
    return out;
  }

  // Choose the sign that avoids cancellation, then use u * v = -p / 3.
  const double sqrtDiscriminant = std::sqrt(discriminant);
  const double u = std::cbrt(-halfQ + (halfQ <= 0. ? sqrtDiscriminant : -sqrtDiscriminant));
  const double v = -thirdP / u;
  const double lambdaR = u + v + shift;
  const double lambdaCr = -0.5 * (u + v) + shift;
  const double lambdaCi = 0.5 * std::sqrt(3.) * std::abs(u - v);

  // The real eigenvector spans the null space of J - lambda_r I, which has rank 2,
  // so take the largest cross product of two of its rows.
  const double rows[3][3] = { { j[0] - lambdaR, j[1], j[2] }, { j[3], j[4] - lambdaR, j[5] },
    { j[6], j[7], j[8] - lambdaR } };
  double r[3] = { 0., 0., 0. };
  double rNorm2 = 0.;
  for (int a = 0; a < 3; ++a)
  {
    const double* x = rows[a];
    const double* y = rows[(a + 1) % 3];
    const double c[3] = { x[1] * y[2] - x[2] * y[1], x[2] * y[0] - x[0] * y[2],
      x[0] * y[1] - x[1] * y[0] };
    const double cNorm2 = c[0] * c[0] + c[1] * c[1] + c[2] * c[2];
    if (cNorm2 > rNorm2)
    {
      std::copy_n(c, 3, r);
      rNorm2 = cNorm2;
    }
  }
  if (!(rNorm2 > 0.))
  {
    return out;
  }
  const double rNorm = std::sqrt(rNorm2);

  const double vorticity[3] = { j[7] - j[5], j[2] - j[6], j[3] - j[1] };
  double omegaDotR = (vorticity[0] * r[0] + vorticity[1] * r[1] + vorticity[2] * r[2]) / rNorm;
  const double sign = omegaDotR < 0. ? -1. : 1.;
  omegaDotR *= sign;

  const double radicand = std::max(0., omegaDotR * omegaDotR - 4. * lambdaCi * lambdaCi);
  out.Magnitude = omegaDotR - std::sqrt(radicand);
  for (int c = 0; c < 3; ++c)
  {
    out.Vector[c] = out.Magnitude * sign * r[c] / rNorm;
  }

  out.Beta2 = 0.25 * omegaDotR * omegaDotR;
  out.LambdaCi2 = lambdaCi * lambdaCi;
  const double alpha2 = 0.25 * radicand;
  out.OmegaDenominator = out.Beta2 + alpha2 + lambdaCr * lambdaCr + 0.5 * lambdaR * lambdaR;
  return out;
}

template <typename JArrayT, typename OutArrayT>
struct LiutexFunctor
{
  JArrayT* Jacobian;
  OutArrayT* Vectors;
  OutArrayT* Magnitudes;
  std::vector<double>* Beta2;
  std::vector<double>* OmegaDenominators;
  vtkLiutex* Filter;
  vtkSMPThreadLocal<double> LocalMaxLambdaCi2;
  double MaxLambdaCi2 = 0.;

  LiutexFunctor(JArrayT* jacobian, OutArrayT* vectors, OutArrayT* magnitudes,
    std::vector<double>* beta2, std::vector<double>* omegaDenominators, vtkLiutex* filter)
    : Jacobian(jacobian)
    , Vectors(vectors)
    , Magnitudes(magnitudes)
    , Beta2(beta2)
    , OmegaDenominators(omegaDenominators)
    , Filter(filter)
  {
  }

  void Initialize() { this->LocalMaxLambdaCi2.Local() = 0.; }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    const auto jRange = vtk::DataArrayTupleRange<9>(this->Jacobian, begin, end);
    double& threadMax = this->LocalMaxLambdaCi2.Local();
    const bool isFirst = vtkSMPTools::GetSingleThread();
    double j[9];
    vtkIdType id = begin;
    for (const auto tuple : jRange)
    {
      if (isFirst)
      {
        this->Filter->CheckAbort();
      }
      if (this->Filter->GetAbortOutput())
      {
        break;
      }
      std::copy(tuple.begin(), tuple.end(), j);
      const LiutexValues values = ComputeLiutex(j);
      if (this->Vectors)
      {
        for (int c = 0; c < 3; ++c)
        {
          this->Vectors->SetTypedComponent(id, c, values.Vector[c]);
        }
      }
      if (this->Magnitudes)
      {
        this->Magnitudes->SetTypedComponent(id, 0, values.Magnitude);
      }
      if (this->Beta2)
      {
        (*this->Beta2)[id] = values.Beta2;
        (*this->OmegaDenominators)[id] = values.OmegaDenominator;
      }
      threadMax = std::max(threadMax, values.LambdaCi2);
      ++id;
    }
  }

  void Reduce()
  {
    for (const double value : this->LocalMaxLambdaCi2)
    {
      this->MaxLambdaCi2 = std::max(this->MaxLambdaCi2, value);
    }
  }
};

template <typename OutArrayT>
struct LiutexWorker
{
  template <typename JArrayT>
  void operator()(JArrayT* jacobian, OutArrayT* vectors, OutArrayT* magnitudes,
    std::vector<double>* beta2, std::vector<double>* omegaDenominators, double& maxLambdaCi2,
    vtkLiutex* filter)
  {
    LiutexFunctor<JArrayT, OutArrayT> functor(
      jacobian, vectors, magnitudes, beta2, omegaDenominators, filter);
    vtkSMPTools::For(0, jacobian->GetNumberOfTuples(), functor);
    maxLambdaCi2 = std::max(maxLambdaCi2, functor.MaxLambdaCi2);
  }
};

template <typename OutArrayT>
void RunLiutex(vtkDataArray* jArray, vtkDataArray* vectors, vtkDataArray* magnitudes,
  std::vector<double>* beta2, std::vector<double>* omegaDenominators, double& maxLambdaCi2,
  vtkLiutex* filter)
{
  LiutexWorker<OutArrayT> worker;
  OutArrayT* typedVectors = vectors ? OutArrayT::FastDownCast(vectors) : nullptr;
  OutArrayT* typedMagnitudes = magnitudes ? OutArrayT::FastDownCast(magnitudes) : nullptr;
  using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
  if (!Dispatcher::Execute(jArray, worker, typedVectors, typedMagnitudes, beta2, omegaDenominators,
        maxLambdaCi2, filter))
  {
    worker(jArray, typedVectors, typedMagnitudes, beta2, omegaDenominators, maxLambdaCi2, filter);
  }
}

vtkDataSetAttributes* GetAttributes(vtkDataSet* dataSet, int association)
{
  if (association == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    return dataSet->GetCellData();
  }
  return dataSet->GetPointData();
}
}

VTK_ABI_NAMESPACE_BEGIN
struct vtkLiutex::BlockState
{
  vtkSmartPointer<vtkDataArray> OmegaLiutex;
  std::vector<double> Beta2;
  std::vector<double> OmegaDenominators;
};

//------------------------------------------------------------------------------
vtkLiutex::vtkLiutex()
{
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, vtkDataSetAttributes::VECTORS);
}

//------------------------------------------------------------------------------
vtkLiutex::~vtkLiutex() = default;

//------------------------------------------------------------------------------
int vtkLiutex::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//------------------------------------------------------------------------------
bool vtkLiutex::ProcessDataSet(vtkDataSet* dataSet, BlockState& state, double& maxLambdaCi2)
{
  int association = vtkDataObject::FIELD_ASSOCIATION_POINTS;
  vtkDataArray* inputArray = this->GetInputArrayToProcess(0, dataSet, association);
  if (!inputArray)
  {
    vtkErrorMacro("No input array to process.");
    return false;
  }

  vtkSmartPointer<vtkDataArray> jacobian;
  if (inputArray->GetNumberOfComponents() == 9)
  {
    jacobian = inputArray;
  }
  else if (inputArray->GetNumberOfComponents() == 3)
  {
    if (!inputArray->GetName())
    {
      vtkErrorMacro("The input velocity array must have a name.");
      return false;
    }
    vtkNew<vtkGradientFilter> gradient;
    gradient->SetInputData(dataSet);
    gradient->SetInputArrayToProcess(0, 0, 0, association, inputArray->GetName());
    gradient->SetResultArrayName("vtkLiutexVelocityGradient");
    gradient->SetFasterApproximation(this->FasterApproximation);
    gradient->SetContainerAlgorithm(this);
    gradient->Update();
    jacobian =
      GetAttributes(gradient->GetOutput(), association)->GetArray("vtkLiutexVelocityGradient");
    if (!jacobian)
    {
      vtkErrorMacro("Could not compute the velocity gradient.");
      return false;
    }
  }
  else
  {
    vtkErrorMacro("The input array must be a 3-component velocity or a 9-component velocity "
                  "gradient, but \""
      << (inputArray->GetName() ? inputArray->GetName() : "") << "\" has "
      << inputArray->GetNumberOfComponents() << " components.");
    return false;
  }

  const vtkIdType numberOfTuples = jacobian->GetNumberOfTuples();
  const bool useDouble = jacobian->GetDataType() == VTK_DOUBLE;
  auto newArray = [&](const std::string& name, int numberOfComponents)
  {
    vtkSmartPointer<vtkDataArray> array;
    if (useDouble)
    {
      array = vtkSmartPointer<vtkDoubleArray>::New();
    }
    else
    {
      array = vtkSmartPointer<vtkFloatArray>::New();
    }
    array->SetName(name.c_str());
    array->SetNumberOfComponents(numberOfComponents);
    array->SetNumberOfTuples(numberOfTuples);
    return array;
  };

  vtkSmartPointer<vtkDataArray> vectors =
    this->ComputeLiutexVector ? newArray(this->LiutexVectorArrayName, 3) : nullptr;
  vtkSmartPointer<vtkDataArray> magnitudes =
    this->ComputeLiutexMagnitude ? newArray(this->LiutexMagnitudeArrayName, 1) : nullptr;
  if (this->ComputeOmegaLiutex)
  {
    state.OmegaLiutex = newArray(this->OmegaLiutexArrayName, 1);
    state.Beta2.resize(numberOfTuples);
    state.OmegaDenominators.resize(numberOfTuples);
  }
  std::vector<double>* beta2 = this->ComputeOmegaLiutex ? &state.Beta2 : nullptr;
  std::vector<double>* denominators = this->ComputeOmegaLiutex ? &state.OmegaDenominators : nullptr;

  if (useDouble)
  {
    RunLiutex<vtkDoubleArray>(
      jacobian, vectors, magnitudes, beta2, denominators, maxLambdaCi2, this);
  }
  else
  {
    RunLiutex<vtkFloatArray>(
      jacobian, vectors, magnitudes, beta2, denominators, maxLambdaCi2, this);
  }

  vtkDataSetAttributes* attributes = GetAttributes(dataSet, association);
  if (vectors)
  {
    attributes->AddArray(vectors);
  }
  if (magnitudes)
  {
    attributes->AddArray(magnitudes);
  }
  if (state.OmegaLiutex)
  {
    attributes->AddArray(state.OmegaLiutex);
  }
  return true;
}

//------------------------------------------------------------------------------
int vtkLiutex::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* output = vtkDataObject::GetData(outputVector, 0);

  std::vector<BlockState> blocks;
  double maxLambdaCi2 = 0.;
  bool success = true;

  if (auto inputDataSet = vtkDataSet::SafeDownCast(input))
  {
    auto outputDataSet = vtkDataSet::SafeDownCast(output);
    outputDataSet->ShallowCopy(inputDataSet);
    blocks.emplace_back();
    success = outputDataSet->GetNumberOfPoints() == 0 ||
      this->ProcessDataSet(outputDataSet, blocks.back(), maxLambdaCi2);
  }
  else if (auto inputComposite = vtkCompositeDataSet::SafeDownCast(input))
  {
    auto outputComposite = vtkCompositeDataSet::SafeDownCast(output);
    outputComposite->CopyStructure(inputComposite);
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(inputComposite->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      auto block = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (!block)
      {
        continue;
      }
      vtkSmartPointer<vtkDataSet> outputBlock;
      outputBlock.TakeReference(block->NewInstance());
      outputBlock->ShallowCopy(block);
      outputComposite->SetDataSet(iter, outputBlock);
      if (outputBlock->GetNumberOfPoints() == 0)
      {
        continue;
      }
      blocks.emplace_back();
      if (!this->ProcessDataSet(outputBlock, blocks.back(), maxLambdaCi2))
      {
        success = false;
        break;
      }
    }
  }
  else
  {
    vtkErrorMacro("Unsupported input type " << input->GetClassName());
    success = false;
  }

  if (this->ComputeOmegaLiutex)
  {
    // Every process must take part in the reduction, including one that
    // failed above, or the others wait for it indefinitely.
    vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
    if (this->OmegaLiutexEpsilon < 0. && controller && controller->GetNumberOfProcesses() > 1)
    {
      double localMax = maxLambdaCi2;
      controller->AllReduce(&localMax, &maxLambdaCi2, 1, vtkCommunicator::MAX_OP);
    }
    if (!success)
    {
      return 0;
    }
    const double epsilon = this->OmegaLiutexEpsilon >= 0. ? this->OmegaLiutexEpsilon
                                                          : this->OmegaLiutexB0 * maxLambdaCi2;
    for (auto& block : blocks)
    {
      if (!block.OmegaLiutex)
      {
        continue;
      }
      vtkDataArray* omega = block.OmegaLiutex;
      vtkSMPTools::For(0, omega->GetNumberOfTuples(),
        [&](vtkIdType begin, vtkIdType end)
        {
          for (vtkIdType i = begin; i < end; ++i)
          {
            const double denominator = block.OmegaDenominators[i] + epsilon;
            omega->SetComponent(i, 0, denominator > 0. ? block.Beta2[i] / denominator : 0.);
          }
        });
    }
  }

  return success ? 1 : 0;
}

//------------------------------------------------------------------------------
void vtkLiutex::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ComputeLiutexVector: " << this->ComputeLiutexVector << endl;
  os << indent << "ComputeLiutexMagnitude: " << this->ComputeLiutexMagnitude << endl;
  os << indent << "ComputeOmegaLiutex: " << this->ComputeOmegaLiutex << endl;
  os << indent << "LiutexVectorArrayName: " << this->LiutexVectorArrayName << endl;
  os << indent << "LiutexMagnitudeArrayName: " << this->LiutexMagnitudeArrayName << endl;
  os << indent << "OmegaLiutexArrayName: " << this->OmegaLiutexArrayName << endl;
  os << indent << "OmegaLiutexB0: " << this->OmegaLiutexB0 << endl;
  os << indent << "OmegaLiutexEpsilon: " << this->OmegaLiutexEpsilon << endl;
  os << indent << "FasterApproximation: " << this->FasterApproximation << endl;
}
VTK_ABI_NAMESPACE_END
