// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkVortexCore.h"

#include "vtkArrayCalculator.h"
#include "vtkArrayDispatch.h"
#include "vtkCharArray.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkGradientFilter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkParallelVectors.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"

#include "vtk_eigen.h"
#include VTK_EIGEN(Eigenvalues)
#include VTK_EIGEN(Geometry)

#include <array>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
namespace
{
// Computes A*b = x given a 3-matrix A and a 3-vector b.
template <typename AArrayType, typename BArrayType, typename XArrayType>
class MatrixVectorMultiplyFunctor
{
  AArrayType* AArray;
  BArrayType* BArray;
  XArrayType* XArray;
  vtkVortexCore* Filter;

public:
  MatrixVectorMultiplyFunctor(
    AArrayType* aArray, BArrayType* bArray, XArrayType* xArray, vtkVortexCore* filter)
    : AArray(aArray)
    , BArray(bArray)
    , XArray(xArray)
    , Filter(filter)
  {
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    const auto aRange = vtk::DataArrayTupleRange<9>(this->AArray, begin, end);
    const auto bRange = vtk::DataArrayTupleRange<3>(this->BArray, begin, end);
    auto xRange = vtk::DataArrayTupleRange<3>(this->XArray, begin, end);

    auto a = aRange.cbegin();
    auto b = bRange.cbegin();
    auto x = xRange.begin();
    bool isFirst = vtkSMPTools::GetSingleThread();

    for (; a != aRange.cend(); ++a, ++b, ++x)
    {
      if (isFirst)
      {
        this->Filter->CheckAbort();
      }
      if (this->Filter->GetAbortOutput())
      {
        break;
      }
      for (vtkIdType i = 0; i < 3; ++i)
      {
        (*x)[i] =
          ((*a)[0 + i * 3] * (*b)[0] + (*a)[1 + i * 3] * (*b)[1] + (*a)[2 + i * 3] * (*b)[2]);
      }
    }
  }
};

struct MatrixVectorMultiplyWorker
{
  template <typename AArrayType, typename BArrayType, typename XArrayType>
  void operator()(AArrayType* aArray, BArrayType* bArray, XArrayType* xArray, vtkVortexCore* filter)
  {
    MatrixVectorMultiplyFunctor<AArrayType, BArrayType, XArrayType> functor(
      aArray, bArray, xArray, filter);
    vtkSMPTools::For(0, xArray->GetNumberOfTuples(), functor);
  }
};

// Compute the Q-criterion, delta-criterion, and lambda_2-criterion as defined in
// Haller, G. (2005). An objective definition of a vortex. Journal of Fluid
// Mechanics, 525, 1-26. Also compute lambda_ci criterion as defined in
// Chakraborty, P., Balachandar, S., & Adran, R. (2005). On the relationships
// between local vortex identification schemes. Journal of Fluid Mechanics, 535,
// 189-214. Return false if any criterion is not satisfied.
bool computeVortexCriteria(const double s[9], const double omega[9], double vortexCriteria[4],
  const vtkTypeBool computeAdditionalTerms = true)
{
  // The velocity gradient tensor $J_{ij} = \frac{\partial u_i}{\partial x_j}$ can be
  // decomposed into a symmetric and antisymmetric part:
  // J = S + \Omega
  // where $S = \frac{1}{2} \left[ J + J^{T} \right]$ is known as the rate-of-strain
  // tensor and $\Omega= \frac{1}{2} \left[ J - J^{T} \right]$ is known as the
  // vorticity tensor.

  Eigen::Matrix<double, 3, 3> S, Omega, J;
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      const double& s_ij = s[3 * i + j];
      const double& omega_ij = omega[3 * i + j];
      S(i, j) = s_ij;
      Omega(i, j) = omega_ij;
      J(i, j) = (s_ij + omega_ij) / 2.;
    }
  }

  // The Q-criterion is defined as
  // Q = \frac{1}{2} \left[ | \Omega |^2 - | S |^2 \right] > 0
  double& Q = vortexCriteria[0];
  Q = (Omega.operatorNorm() - S.operatorNorm()) / 2.;
  if (Q <= 0.)
  {
    return false;
  }

  // The delta-criterion is defined as
  // \Delta = \left( \frac{Q}{3} \right)^3 + \left( \frac{\det J}{2} \right)^2 > 0
  const double Q_3 = Q / 3.;
  const double jDet_2 = J.determinant() / 2.;
  double& delta = vortexCriteria[1];
  delta = Q_3 * Q_3 * Q_3 + jDet_2 * jDet_2;
  if (delta <= 0.)
  {
    return false;
  }

  if (!computeAdditionalTerms)
  {
    return true;
  }

  // The lambda_2-criterion is defined as
  // \lambda_2 \left( S^2 + \Omega^2 \right) < 0
  // where $\lambda_2$ is the intermediate eigenvalue
  double& lambda_2 = vortexCriteria[2];
  {
    Eigen::Matrix<double, 3, 3> A = S * S + Omega * Omega;
    auto Eigenvalues = A.eigenvalues();
    // Matrix A is symmetric, so its eigenvalues are all real.
    std::array<double, 3> eigenvalues = { Eigenvalues[0].real(), Eigenvalues[1].real(),
      Eigenvalues[2].real() };
    std::nth_element(eigenvalues.begin(), eigenvalues.begin() + 1, eigenvalues.end());
    lambda_2 = eigenvalues[1];
  }

  if (lambda_2 >= 0.)
  {
    return false;
  }

  // The lambda_ci-criterion is defined as the imaginary component of the
  // eigenvalues of the complex conjugate pair of eigenvalues of J
  double& lambda_ci = vortexCriteria[3];
  {
    Eigen::EigenSolver<Eigen::Matrix<double, 3, 3>> eigensolver(J);
    auto eigenvalues = eigensolver.eigenvalues();

    if (std::abs(eigenvalues[0].imag()) > VTK_DBL_EPSILON)
    {
      if ((std::abs(eigenvalues[0].real() - eigenvalues[1].real()) < VTK_DBL_EPSILON &&
            std::abs(eigenvalues[0].imag() + eigenvalues[1].imag()) < VTK_DBL_EPSILON) ||
        (std::abs(eigenvalues[0].real() - eigenvalues[2].real()) < VTK_DBL_EPSILON &&
          std::abs(eigenvalues[0].imag() + eigenvalues[2].imag()) < VTK_DBL_EPSILON))
      {
        lambda_ci = std::abs(eigenvalues[0].imag());
      }
    }
    else if (std::abs(eigenvalues[1].imag()) > VTK_DBL_EPSILON)
    {
      if (std::abs(eigenvalues[1].real() - eigenvalues[2].real()) < VTK_DBL_EPSILON &&
        std::abs(eigenvalues[1].imag() + eigenvalues[2].imag()) < VTK_DBL_EPSILON)

      {
        lambda_ci = std::abs(eigenvalues[1].imag());
      }
    }
  }

  return true;
}

template <typename JacobianArrayType, typename AcceptedPointsArrayType>
class ComputeCriteriaFunctor
{
  JacobianArrayType* JacobianArray;
  AcceptedPointsArrayType* AcceptedPointsArray;
  vtkVortexCore* Filter;

public:
  ComputeCriteriaFunctor(JacobianArrayType* jacobianArray,
    AcceptedPointsArrayType* acceptedPointsArray, vtkVortexCore* filter)
    : JacobianArray(jacobianArray)
    , AcceptedPointsArray(acceptedPointsArray)
    , Filter(filter)
  {
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    const auto jacobianRange = vtk::DataArrayTupleRange<9>(this->JacobianArray, begin, end);
    auto acceptedPointsRange = vtk::DataArrayValueRange<1>(this->AcceptedPointsArray, begin, end);

    auto j = jacobianRange.cbegin();
    auto a = acceptedPointsRange.begin();
    bool isFirst = vtkSMPTools::GetSingleThread();

    for (; j != jacobianRange.cend(); ++j, ++a)
    {
      if (isFirst)
      {
        this->Filter->CheckAbort();
      }
      if (this->Filter->GetAbortOutput())
      {
        break;
      }
      std::array<double, 4> vortexCriteria;
      double S[9];
      double Omega[9];
      static const std::array<typename decltype(j)::value_type::size_type, 9> idxT = { 0, 3, 6, 1,
        4, 7, 2, 5, 8 };
      for (int i = 0; i < 9; i++)
      {
        double j_i = (*j)[i];

        double jt_i = (*j)[idxT[i]];

        S[i] = (j_i + jt_i) / 2.;
        Omega[i] = (j_i - jt_i) / 2.;
      }
      // Only use the first two criteria to discriminate points
      *a = computeVortexCriteria(S, Omega, vortexCriteria.data(), false);
    }
  }
};

struct ComputeCriteriaWorker
{
  template <typename JacobianArrayType, typename AcceptedPointsArrayType>
  void operator()(JacobianArrayType* jacobianArray, AcceptedPointsArrayType* acceptedPointsArray,
    vtkVortexCore* filter)
  {
    ComputeCriteriaFunctor<JacobianArrayType, AcceptedPointsArrayType> functor(
      jacobianArray, acceptedPointsArray, filter);
    vtkSMPTools::For(0, acceptedPointsArray->GetNumberOfTuples(), functor);
  }
};
}

class vtkParallelVectorsForVortexCore : public vtkParallelVectors
{
public:
  static vtkParallelVectorsForVortexCore* New();
  vtkTypeMacro(vtkParallelVectorsForVortexCore, vtkParallelVectors);

  void SetAcceptedPointsArray(vtkSmartPointer<vtkCharArray>& array)
  {
    this->AcceptedPoints = array;
  }
  void SetJacobianDataArray(vtkSmartPointer<vtkDataArray>& jacobian) { this->Jacobian = jacobian; }

protected:
  vtkParallelVectorsForVortexCore() = default;
  ~vtkParallelVectorsForVortexCore() override = default;

  void Prefilter(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  bool AcceptSurfaceTriangle(const vtkIdType surfaceSimplexIndices[3]) override;

  bool ComputeAdditionalCriteria(const vtkIdType surfaceSimplexIndices[3], double s, double t,
    std::vector<double>& criterionArrayValues) override;

  vtkSmartPointer<vtkCharArray> AcceptedPoints;
  vtkSmartPointer<vtkDataArray> Jacobian;

private:
  vtkParallelVectorsForVortexCore(const vtkParallelVectorsForVortexCore&) = delete;
  void operator=(const vtkParallelVectorsForVortexCore&) = delete;
};
vtkStandardNewMacro(vtkParallelVectorsForVortexCore);

//------------------------------------------------------------------------------
void vtkParallelVectorsForVortexCore::Prefilter(
  vtkInformation*, vtkInformationVector**, vtkInformationVector*)
{
  this->CriteriaArrays.resize(4);
  this->CriteriaArrays[0] = vtkSmartPointer<vtkDoubleArray>::New();
  this->CriteriaArrays[0]->SetName("q-criterion");
  this->CriteriaArrays[1] = vtkSmartPointer<vtkDoubleArray>::New();
  this->CriteriaArrays[1]->SetName("delta-criterion");
  this->CriteriaArrays[2] = vtkSmartPointer<vtkDoubleArray>::New();
  this->CriteriaArrays[2]->SetName("lambda_2-criterion");
  this->CriteriaArrays[3] = vtkSmartPointer<vtkDoubleArray>::New();
  this->CriteriaArrays[3]->SetName("lambda_ci-criterion");
}

//------------------------------------------------------------------------------
bool vtkParallelVectorsForVortexCore::AcceptSurfaceTriangle(
  const vtkIdType surfaceSimplexIndices[3])
{
  auto acceptedPoints = this->AcceptedPoints->GetPointer(0);
  return acceptedPoints[surfaceSimplexIndices[0]] && acceptedPoints[surfaceSimplexIndices[1]] &&
    acceptedPoints[surfaceSimplexIndices[2]];
}

//------------------------------------------------------------------------------
bool vtkParallelVectorsForVortexCore::ComputeAdditionalCriteria(
  const vtkIdType surfaceSimplexIndices[3], double s, double t,
  std::vector<double>& criterionArrayValues)
{
  double j[3][9];
  for (int i = 0; i < 3; i++)
  {
    this->Jacobian->GetTuple(surfaceSimplexIndices[i], j[i]);
  }

  double S[9];
  double Omega[9];
  static const std::array<std::size_t, 9> idxT = { 0, 3, 6, 1, 4, 7, 2, 5, 8 };
  for (int i = 0; i < 9; i++)
  {
    double j_i = (1. - s - t) * j[0][i] + s * j[1][i] + t * j[2][i];

    double jt_i = (1. - s - t) * j[0][idxT[i]] + s * j[1][idxT[i]] + t * j[2][idxT[i]];

    S[i] = (j_i + jt_i) / 2.;
    Omega[i] = (j_i - jt_i) / 2.;
  }

  // If any of the criteria fail, do not add this point
  return computeVortexCriteria(S, Omega, criterionArrayValues.data());
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkVortexCore);

//------------------------------------------------------------------------------
vtkVortexCore::vtkVortexCore()
  : HigherOrderMethod(false)
  , FasterApproximation(false)
{
}

//------------------------------------------------------------------------------
vtkVortexCore::~vtkVortexCore() = default;

//------------------------------------------------------------------------------
int vtkVortexCore::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkPolyData* output = vtkPolyData::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataArray* velocity = this->GetInputArrayToProcess(0, input);

  if (velocity == nullptr)
  {
    vtkErrorMacro(<< "Could not access input vector field");
    return 0;
  }

  vtkSmartPointer<vtkDataSet> dataset;

  // Compute the jacobian from the velocity field
  vtkSmartPointer<vtkDataArray> jacobian;
  {
    vtkNew<vtkGradientFilter> gradient;
    gradient->SetInputData(input);
    gradient->SetFasterApproximation(this->FasterApproximation);
    gradient->SetResultArrayName("jacobian");
    gradient->ComputeVorticityOn();
    gradient->SetVorticityArrayName("vorticity");
    gradient->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, velocity->GetName());
    gradient->SetContainerAlgorithm(this);
    gradient->Update();

    dataset = gradient->GetOutput();

    jacobian = vtkDataArray::SafeDownCast(dataset->GetPointData()->GetAbstractArray("jacobian"));
  }

  if (this->CheckAbort())
  {
    return 1;
  }

  // Compute the acceleration field: a = J * v
  vtkSmartPointer<vtkDoubleArray> acceleration;
  {
    acceleration = vtkSmartPointer<vtkDoubleArray>::New();
    acceleration->SetName("acceleration");
    acceleration->SetNumberOfComponents(3);
    acceleration->SetNumberOfTuples(velocity->GetNumberOfTuples());

    MatrixVectorMultiplyWorker worker;

    // Create a dispatcher. We want to generate fast-paths for when
    // vecs and mags both use doubles or floats, but fallback to a
    // slow path for any other situation.
    using Dispatcher = vtkArrayDispatch::Dispatch3ByValueType<vtkArrayDispatch::Reals,
      vtkArrayDispatch::Reals, vtkArrayDispatch::Reals>;

    // Generate optimized workers when mags/vecs are both float|double
    if (!Dispatcher::Execute(jacobian, velocity, acceleration, worker, this))
    {
      // Otherwise fallback to using the vtkDataArray API.
      worker(jacobian.Get(), velocity, acceleration.Get(), this);
    }

    dataset->GetPointData()->AddArray(acceleration);
    dataset->GetPointData()->SetActiveVectors(acceleration->GetName());
  }

  if (this->CheckAbort())
  {
    return 1;
  }

  vtkDataArray* vField = velocity;
  vtkDataArray* wField = acceleration;

  if (this->HigherOrderMethod)
  {
    // Compute the gradient of the Jacobian
    vtkSmartPointer<vtkDoubleArray> jacobianPrime;
    {
      vtkNew<vtkGradientFilter> gradientPrime;
      gradientPrime->SetInputData(dataset);
      gradientPrime->SetFasterApproximation(this->FasterApproximation);
      gradientPrime->SetResultArrayName("jacobian_prime");
      gradientPrime->SetInputArrayToProcess(
        0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "jacobian");
      gradientPrime->SetContainerAlgorithm(this);
      gradientPrime->Update();
      jacobianPrime = vtkDoubleArray::SafeDownCast(
        gradientPrime->GetOutput()->GetPointData()->GetAbstractArray("jacobian_prime"));
    }

    // Next, compute the jerk field: j = J' * v
    vtkSmartPointer<vtkDoubleArray> jerk;
    {
      jerk = vtkSmartPointer<vtkDoubleArray>::New();
      jerk->SetName("jerk");
      jerk->SetNumberOfComponents(3);
      jerk->SetNumberOfTuples(velocity->GetNumberOfTuples());

      MatrixVectorMultiplyWorker worker;

      using Dispatcher = vtkArrayDispatch::Dispatch3ByValueType<vtkArrayDispatch::Reals,
        vtkArrayDispatch::Reals, vtkArrayDispatch::Reals>;

      // Generate optimized workers when mags/vecs are both float|double
      if (!Dispatcher::Execute(jacobianPrime, velocity, jerk, worker, this))
      {
        // Otherwise, fallback to using the vtkDataArray API.
        worker(jacobianPrime.Get(), velocity, jerk.Get(), this);
      }
    }

    dataset->GetPointData()->AddArray(jerk);
    wField = jerk;
  }

  if (this->CheckAbort())
  {
    return 1;
  }

  // Use criteria to assign acceptance value to each point in the dataset.
  // This worker will be run on all points, so we only use the first two
  // criteria (as they are computationally less expensive).
  vtkSmartPointer<vtkCharArray> acceptedPoints;
  {
    acceptedPoints = vtkSmartPointer<vtkCharArray>::New();
    acceptedPoints->SetNumberOfTuples(jacobian->GetNumberOfTuples());

    ComputeCriteriaWorker worker;
    using Dispatcher =
      vtkArrayDispatch::Dispatch2ByValueType<vtkArrayDispatch::Reals, vtkArrayDispatch::Integrals>;

    if (!Dispatcher::Execute(jacobian, acceptedPoints, worker, this))
    {
      worker(jacobian.Get(), acceptedPoints.Get(), this);
    }
  }
  auto vorticityArray = dataset->GetPointData()->GetArray("vorticity");

  // Compute polylines that correspond to locations where two vector point
  // fields are parallel.
  vtkNew<vtkParallelVectorsForVortexCore> parallelVectorsForVortexCore;
  parallelVectorsForVortexCore->SetInputData(dataset);
  parallelVectorsForVortexCore->SetAcceptedPointsArray(acceptedPoints);
  parallelVectorsForVortexCore->SetJacobianDataArray(jacobian);
  parallelVectorsForVortexCore->SetFirstVectorFieldName(vField->GetName());
  parallelVectorsForVortexCore->SetSecondVectorFieldName(wField->GetName());

  // compute the magnitude of the vorticity array
  vtkNew<vtkArrayCalculator> calculator;
  calculator->SetInputConnection(parallelVectorsForVortexCore->GetOutputPort());
  if (vorticityArray)
  {
    calculator->SetResultArrayType(vorticityArray->GetDataType());
  }
  calculator->AddVectorArrayName("vorticity");
  calculator->SetResultArrayName("vorticity_magnitude");
  calculator->SetFunction("mag(vorticity)");
  calculator->Update();
  output->ShallowCopy(calculator->GetOutput());

  return 1;
}

//------------------------------------------------------------------------------
int vtkVortexCore::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtkVortexCore::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
