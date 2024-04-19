// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPointSmoothingFilter.h"

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkStaticPointLocator.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPointSmoothingFilter);

vtkCxxSetObjectMacro(vtkPointSmoothingFilter, FrameFieldArray, vtkDataArray);
vtkCxxSetObjectMacro(vtkPointSmoothingFilter, Locator, vtkAbstractPointLocator);
vtkCxxSetObjectMacro(vtkPointSmoothingFilter, Plane, vtkPlane);

//------------------------------------------------------------------------------
namespace
{
// Used when tensors need to be padded out to 9-components
template <typename DataT>
struct PadFrameFieldArray
{
  DataT* InTensors;   // 6-component tuples
  double* OutTensors; // 9-component padded tensors

  PadFrameFieldArray(DataT* tIn, double* tOut)
    : InTensors(tIn)
    , OutTensors(tOut)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    double* tensor = this->OutTensors + 9 * ptId;
    const auto tensors = vtk::DataArrayTupleRange<6>(this->InTensors, ptId, endPtId);
    for (const auto tuple : tensors)
    {
      for (auto i = 0; i < 6; ++i)
      {
        tensor[i] = tuple[i];
      }
      vtkMath::TensorFromSymmetricTensor(tensor);
      ++ptId;      // move to the next point
      tensor += 9; // move to next output tensor
    }
  }
};

//------------------------------------------------------------------------------
// Machinery for extracting eigenfunctions. Needed if smoothing mode is set
// to Tensors.
template <typename DataT>
struct ExtractEigenfunctions
{
  DataT* InTensors;
  double* OutTensors; // 9-component tensors with eigenfunctions extracted

  ExtractEigenfunctions(DataT* tIn, double* tOut)
    : InTensors(tIn)
    , OutTensors(tOut)
  {
  }

  void Extract(double* tensor, double* eTensor)
  {
    double *m[3], w[3], *v[3];
    double m0[3], m1[3], m2[3];
    double v0[3], v1[3], v2[3];

    // set up working matrices
    m[0] = m0;
    m[1] = m1;
    m[2] = m2;
    v[0] = v0;
    v[1] = v1;
    v[2] = v2;

    // We are interested in the symmetrical part of the tensor only, since
    // eigenvalues are real if and only if the matrice of reals is symmetrical
    for (auto j = 0; j < 3; j++)
    {
      for (auto i = 0; i < 3; i++)
      {
        m[i][j] = 0.5 * (tensor[i + 3 * j] + tensor[j + 3 * i]);
      }
    }

    vtkMath::Jacobi(m, w, v);

    // copy non-normalized eigenvectors
    eTensor[0] = w[0] * v[0][0];
    eTensor[1] = w[0] * v[1][0];
    eTensor[2] = w[0] * v[2][0];
    eTensor[3] = w[1] * v[0][1];
    eTensor[4] = w[1] * v[1][1];
    eTensor[5] = w[1] * v[2][1];
    eTensor[6] = w[2] * v[0][2];
    eTensor[7] = w[2] * v[1][2];
    eTensor[8] = w[2] * v[2][2];
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    double tensor[9];
    double* t = this->OutTensors + 9 * ptId;

    if (this->InTensors->GetNumberOfComponents() == 9)
    {
      const auto tensors = vtk::DataArrayTupleRange<9>(this->InTensors, ptId, endPtId);
      for (const auto tuple : tensors)
      {
        for (auto i = 0; i < 9; ++i)
        {
          tensor[i] = tuple[i];
        }
        this->Extract(tensor, t);
        ++ptId; // move to the next point
        t += 9; // move to next output tensor
      }
    }
    else // 6 component symmetric tensor
    {
      const auto tensors = vtk::DataArrayTupleRange<6>(this->InTensors, ptId, endPtId);
      for (const auto tuple : tensors)
      {
        for (auto i = 0; i < 6; ++i)
        {
          tensor[i] = tuple[i];
        }
        vtkMath::TensorFromSymmetricTensor(tensor);
        this->Extract(tensor, t);
        ++ptId; // move to the next point
        t += 9; // move to next output tensor
      }
    }
  }
};

// Hooks into dispatcher vtkArrayDispatch by providing a callable generic
struct FrameFieldWorker
{
  vtkDoubleArray* PaddedTensors;
  FrameFieldWorker() { this->PaddedTensors = vtkDoubleArray::New(); }

  template <typename DataT>
  void operator()(DataT* tensors)
  {
    vtkIdType numPts = tensors->GetNumberOfTuples();
    this->PaddedTensors->SetNumberOfComponents(9);
    this->PaddedTensors->SetNumberOfTuples(numPts);
    PadFrameFieldArray<DataT> pad(tensors, this->PaddedTensors->GetPointer(0));
    vtkSMPTools::For(0, numPts, pad);
  }
};

// Centralize the dispatch to avoid duplication
vtkDataArray* PadFrameField(vtkDataArray* tensors)
{
  if (tensors->GetNumberOfComponents() == 9)
  {
    // Register needed as no new array is created, but expected
    tensors->Register(nullptr);
    return tensors;
  }
  else
  {
    using vtkArrayDispatch::Reals;
    using PadFrameFieldDispatch = vtkArrayDispatch::DispatchByValueType<Reals>;
    FrameFieldWorker padWorker;
    if (!PadFrameFieldDispatch::Execute(tensors, padWorker))
    { // Fallback to slowpath for other point types
      padWorker(tensors);
    }
    return padWorker.PaddedTensors;
  }
}

// Hooks into dispatcher vtkArrayDispatch by providing a callable generic
struct EigenWorker
{
  vtkDoubleArray* Eigens;

  EigenWorker() { this->Eigens = vtkDoubleArray::New(); }

  template <typename DataT>
  void operator()(DataT* tensors)
  {
    vtkIdType numPts = tensors->GetNumberOfTuples();
    this->Eigens->SetNumberOfComponents(9);
    this->Eigens->SetNumberOfTuples(numPts);
    ExtractEigenfunctions<DataT> extract(tensors, this->Eigens->GetPointer(0));
    vtkSMPTools::For(0, numPts, extract);
  }
};

// Centralize the dispatch to avoid duplication
vtkDataArray* ComputeEigenvalues(vtkDataArray* tensors)
{
  using vtkArrayDispatch::Reals;
  using EigenDispatch = vtkArrayDispatch::DispatchByValueType<Reals>;
  EigenWorker eigenWorker;
  if (!EigenDispatch::Execute(tensors, eigenWorker))
  { // Fallback to slowpath for other point types
    eigenWorker(tensors);
  }
  return eigenWorker.Eigens;
}

// Determine the min/max determinant values of the tensor field
template <typename TensorT>
struct CharacterizeTensors
{
  TensorT* Tensors;
  double DeterminantRange[2];

  vtkSMPThreadLocal<double> LocalDetMin;
  vtkSMPThreadLocal<double> LocalDetMax;

  CharacterizeTensors(TensorT* tensors)
    : Tensors(tensors)
  {
  }

  void Initialize()
  {
    this->LocalDetMin.Local() = VTK_DOUBLE_MAX;
    this->LocalDetMax.Local() = VTK_DOUBLE_MIN;
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    double& min = this->LocalDetMin.Local();
    double& max = this->LocalDetMax.Local();
    double det;

    if (this->Tensors->GetNumberOfComponents() == 9)
    {
      const auto tensors = vtk::DataArrayTupleRange<9>(this->Tensors, ptId, endPtId);
      for (const auto tensor : tensors)
      {
        det = fabs(tensor[0] * tensor[4] * tensor[8] - tensor[0] * tensor[5] * tensor[7] -
          tensor[1] * tensor[3] * tensor[8] + tensor[1] * tensor[5] * tensor[6] +
          tensor[2] * tensor[3] * tensor[7] - tensor[2] * tensor[4] * tensor[6]);

        min = std::min(det, min);
        max = std::max(det, max);
      }
    }
    else // 6-component symmetric tensor
    {
      const auto tensors = vtk::DataArrayTupleRange<6>(this->Tensors, ptId, endPtId);
      double tensor[9];
      for (const auto tuple : tensors)
      {
        for (auto i = 0; i < 6; ++i)
        {
          tensor[i] = tuple[i];
        }
        vtkMath::TensorFromSymmetricTensor(tensor);

        det = fabs(tensor[0] * tensor[4] * tensor[8] - tensor[0] * tensor[5] * tensor[7] -
          tensor[1] * tensor[3] * tensor[8] + tensor[1] * tensor[5] * tensor[6] +
          tensor[2] * tensor[3] * tensor[7] - tensor[2] * tensor[4] * tensor[6]);

        min = std::min(det, min);
        max = std::max(det, max);
      }
    }
  }

  void Reduce()
  {
    double min = VTK_DOUBLE_MAX;
    double max = VTK_DOUBLE_MIN;
    for (auto iter = this->LocalDetMin.begin(); iter != this->LocalDetMin.end(); ++iter)
    {
      min = std::min(*iter, min);
    }
    for (auto iter = this->LocalDetMax.begin(); iter != this->LocalDetMax.end(); ++iter)
    {
      max = std::max(*iter, max);
    }
    this->DeterminantRange[0] = min;
    this->DeterminantRange[1] = max;
  }
};

// Hooks into dispatcher vtkArrayDispatch by providing a callable generic
struct TensorWorker
{
  double Range[2]; // min/max value of determinant

  template <typename TensorT>
  void operator()(TensorT* tensors, vtkIdType numPts)
  {
    CharacterizeTensors<TensorT> characterizeTensors(tensors);
    vtkSMPTools::For(0, numPts, characterizeTensors);
    this->Range[0] = characterizeTensors.DeterminantRange[0];
    this->Range[1] = characterizeTensors.DeterminantRange[1];
  }
};

// Centralize the dispatch to avoid duplication
void CharacterizeTensor(vtkDataArray* tensors, vtkIdType numPts, double detRange[2])
{
  using vtkArrayDispatch::Reals;
  using TensorDispatch = vtkArrayDispatch::DispatchByValueType<Reals>;
  TensorWorker tensorWorker;
  if (!TensorDispatch::Execute(tensors, tensorWorker, numPts))
  { // Fallback to slowpath for other point types
    tensorWorker(tensors, numPts);
  }
  detRange[0] = tensorWorker.Range[0];
  detRange[1] = tensorWorker.Range[1];
}

//------------------------------------------------------------------------------
// These classes compute the forced displacement of a point within a
// neighborhood of points. Besides geometric proximity, attribute
// data (e.g., scalars, tensors) may also affect the displacement.
struct DisplacePoint
{
  vtkDataArray* Data;   // data attribute of interest
  double PackingRadius; // radius of average sphere
  double PackingFactor;
  double AttractionFactor;
  vtkNew<vtkMinimalStandardRandomSequence> RandomSeq;

  DisplacePoint(vtkDataArray* data, double radius, double pf, double af)
    : Data(data)
    , PackingRadius(radius)
    , PackingFactor(pf)
    , AttractionFactor(af)
  {
    this->RandomSeq->Initialize(1177);
  }
  virtual ~DisplacePoint() = default; // Needed for subclasses to delete RandomSeq

  // Generate a displacement for the given point from the
  // surrounding neighborhood.
  virtual void operator()(vtkIdType p0, double x[3], vtkIdType numNeis, const vtkIdType* neis,
    const double* neiPts, double disp[3]) = 0;

  // Compute an inter-point force depending on normalized radius. The force
  // is linearly repulsive near the point 0<=r<=1; has a slight (cubic)
  // attractive force in the region (1<r<=(1+af); and produces no force
  // further away.
  inline double ParticleForce(double r, double af)
  {
    double af1 = 1.0 + af;
    if (r <= 1.0) // repulsive, negative force
    {
      return (r - 1.0);
    }
    else if (r > af1) // far away do nothing
    {
      return 0.0;
    }
    else // attractive, positive force
    {
      return ((r - 1.0) * (af1 - r) * (af1 - r) / (af * af));
    }
  }
};
// Nearby points apply forces (not modified by distance nor attribute data)
// This is a form of Laplacian smoothing. Attributes do not affect the
// displacement. This has a tendency to collapse points to the center of
// their local neighborhood.
struct GeometricDisplacement : public DisplacePoint
{
  GeometricDisplacement(vtkDataArray* data, double radius, double pf, double af)
    : DisplacePoint(data, radius, pf, af)
  {
  }
  ~GeometricDisplacement() override = default;

  void operator()(vtkIdType, double x[3], vtkIdType numNeis, const vtkIdType* neis,
    const double* neiPts, double disp[3]) override
  {
    int count = 0;
    double ave[3] = { 0.0, 0.0, 0.0 };
    double len, fVec[3];
    vtkIdType neiId;
    double R = this->PackingFactor * this->PackingRadius;
    for (auto i = 0; i < numNeis; ++i)
    {
      neiId = neis[i];
      // Make sure to have a valid connection within sphere of influence
      if (neiId >= 0)
      {
        fVec[0] = neiPts[3 * i] - x[0];
        fVec[1] = neiPts[3 * i + 1] - x[1];
        fVec[2] = neiPts[3 * i + 2] - x[2];
        if ((len = vtkMath::Normalize(fVec)) <= R)
        {
          ++count;
          ave[0] += neiPts[3 * i];
          ave[1] += neiPts[3 * i + 1];
          ave[2] += neiPts[3 * i + 2];
        }
      }
    }
    if (count <= 0)
    {
      disp[0] = 0.0;
      disp[1] = 0.0;
      disp[2] = 0.0;
    }
    else
    {
      // Displace towards the average of surrounding points
      ave[0] /= static_cast<double>(count);
      ave[1] /= static_cast<double>(count);
      ave[2] /= static_cast<double>(count);
      disp[0] = (ave[0] - x[0]);
      disp[1] = (ave[1] - x[1]);
      disp[2] = (ave[2] - x[2]);
    }
  }
};

// Forces from nearby points are moderated by their distance. Attributes
// do not affect the displacement.
struct UniformDisplacement : public DisplacePoint
{
  UniformDisplacement(vtkDataArray* data, double radius, double pf, double af)
    : DisplacePoint(data, radius, pf, af)
  {
  }
  ~UniformDisplacement() override = default;

  void operator()(vtkIdType, double x[3], vtkIdType numNeis, const vtkIdType* neis,
    const double* neiPts, double disp[3]) override
  {
    double len, force;
    double fVec[3];
    vtkIdType neiId;
    disp[0] = disp[1] = disp[2] = 0.0;
    for (auto i = 0; i < numNeis; ++i)
    {
      neiId = neis[i];
      if (neiId >= 0) // valid connection to another point
      {
        fVec[0] = neiPts[3 * i] - x[0];
        fVec[1] = neiPts[3 * i + 1] - x[1];
        fVec[2] = neiPts[3 * i + 2] - x[2];
        if ((len = vtkMath::Normalize(fVec)) == 0.0)
        { // points coincident, bump them apart
          fVec[0] = this->RandomSeq->GetValue();
          this->RandomSeq->Next();
        }
        force = this->ParticleForce(
          len / (this->PackingFactor * this->PackingRadius), this->AttractionFactor);
        disp[0] += force * fVec[0];
        disp[1] += force * fVec[1];
        disp[2] += force * fVec[2];
      }
    }
  }
};

// Forces on nearby points are moderated by distance and scalar values.
// The local transformation due to scalar is a uniform transformation.
struct ScalarDisplacement : public DisplacePoint
{
  double Range[2];
  double ScalarAverage;

  ScalarDisplacement(vtkDataArray* data, double radius, double pf, double af, double range[2])
    : DisplacePoint(data, radius, pf, af)
  {
    this->Range[0] = range[0];
    this->Range[1] = range[1];
    this->ScalarAverage = (this->Range[0] + this->Range[1]) / 2.0;
  }
  ~ScalarDisplacement() override = default;

  void operator()(vtkIdType p0, double x[3], vtkIdType numNeis, const vtkIdType* neis,
    const double* neiPts, double disp[3]) override
  {
    double len, force, s0, s1, sf;
    double fVec[3];
    vtkIdType neiId;
    disp[0] = disp[1] = disp[2] = 0.0;
    this->Data->GetTuple(p0, &s0);
    for (auto i = 0; i < numNeis; ++i)
    {
      neiId = neis[i];
      if (neiId >= 0) // valid connection to another point
      {
        this->Data->GetTuple(neiId, &s1);
        sf = 1.0 / (0.5 * (s0 + s1)); // average of two scalars
        fVec[0] = neiPts[3 * i] - x[0];
        fVec[1] = neiPts[3 * i + 1] - x[1];
        fVec[2] = neiPts[3 * i + 2] - x[2];
        if ((len = vtkMath::Normalize(fVec)) == 0.0)
        { // points coincident, bump them apart
          fVec[0] = this->RandomSeq->GetValue();
          this->RandomSeq->Next();
        }
        force = this->ParticleForce(
          len / (this->PackingFactor * this->PackingRadius), this->AttractionFactor);
        disp[0] += (sf * force * fVec[0]);
        disp[1] += (sf * force * fVec[1]);
        disp[2] += (sf * force * fVec[2]);
      }
    }
  }
};
// Forces on nearby points are moderated by distance and tensor values.
struct TensorDisplacement : public DisplacePoint
{
  double DetRange[2];

  TensorDisplacement(vtkDataArray* data, double radius, double pf, double af, double detRange[2])
    : DisplacePoint(data, radius, pf, af)
  {
    this->DetRange[0] = detRange[0];
    this->DetRange[1] = detRange[1];
  }
  ~TensorDisplacement() override = default;

  //--------------------------------------------------------------------------
  // Average two 3x3 tensors represented as 9 entries in a contiguous array
  inline void AverageTensors(const double* t0, const double* t1, double* tAve)
  {
    tAve[0] = 0.5 * (t0[0] + t1[0]);
    tAve[1] = 0.5 * (t0[1] + t1[1]);
    tAve[2] = 0.5 * (t0[2] + t1[2]);
    tAve[3] = 0.5 * (t0[3] + t1[3]);
    tAve[4] = 0.5 * (t0[4] + t1[4]);
    tAve[5] = 0.5 * (t0[5] + t1[5]);
    tAve[6] = 0.5 * (t0[6] + t1[6]);
    tAve[7] = 0.5 * (t0[7] + t1[7]);
    tAve[8] = 0.5 * (t0[8] + t1[8]);
  }

  //--------------------------------------------------------------------------
  // Invert 3x3 symmetric, positive definite matrix. Matrices are a pointer to
  // 9 entries in a contiguous array, three columns in order.
  inline void Invert3x3(double* m, double* mI)
  {
    double detF = vtkMath::Determinant3x3(m, m + 3, m + 6);
    if (detF == 0.0)
    {
      mI[0] = mI[1] = mI[2] = mI[3] = mI[4] = mI[5] = mI[6] = mI[7] = mI[8] = 0.0;
      return;
    }

    detF = 1.0 / detF;
    mI[0] = detF * (m[8] * m[4] - m[5] * m[7]);
    mI[1] = detF * (-(m[8] * m[1] - m[2] * m[7]));
    mI[2] = detF * (m[5] * m[1] - m[2] * m[4]);
    mI[3] = detF * (-(m[8] * m[3] - m[5] * m[6]));
    mI[4] = detF * (m[8] * m[0] - m[2] * m[6]);
    mI[5] = detF * (-(m[5] * m[0] - m[2] * m[3]));
    mI[6] = detF * (m[7] * m[3] - m[4] * m[6]);
    mI[7] = detF * (-(m[7] * m[0] - m[1] * m[6]));
    mI[8] = detF * (m[4] * m[0] - m[1] * m[3]);
  }

  void TransformForceVector(const double* tI, const double* fVec, double* newfVec)
  {
    newfVec[0] = tI[0] * fVec[0] + tI[3] * fVec[1] + tI[6] * fVec[2];
    newfVec[1] = tI[1] * fVec[0] + tI[4] * fVec[1] + tI[7] * fVec[2];
    newfVec[2] = tI[2] * fVec[0] + tI[5] * fVec[1] + tI[8] * fVec[2];
  }

  void operator()(vtkIdType pb, double px[3], vtkIdType numNeis, const vtkIdType* neis,
    const double* neiPts, double disp[3]) override
  {
    double twoAlpha = 2.0 * this->PackingRadius * this->PackingFactor;
    double y[3], x[3], r, fVec[3];
    double force, tb[9], ta[9];
    double tAve[9], DI[9];
    disp[0] = disp[1] = disp[2] = 0.0;
    vtkIdType neiId;

    this->Data->GetTuple(pb, tb);
    for (auto i = 0; i < numNeis; ++i)
    {
      neiId = neis[i];
      if (neiId >= 0) // valid connection to another point
      {
        y[0] = neiPts[3 * i] - px[0];
        y[1] = neiPts[3 * i + 1] - px[1];
        y[2] = neiPts[3 * i + 2] - px[2];

        this->Data->GetTuple(neiId, ta);
        this->AverageTensors(ta, tb, tAve);
        this->Invert3x3(tAve, DI);
        this->TransformForceVector(DI, y, x);
        x[0] /= twoAlpha;
        x[1] /= twoAlpha;
        x[2] /= twoAlpha;
        r = vtkMath::Norm(x);

        // Compute final force vector
        force = this->ParticleForce(r, this->AttractionFactor) / (twoAlpha * r);
        this->TransformForceVector(DI, x, fVec);
        disp[0] += (force * fVec[0]);
        disp[1] += (force * fVec[1]);
        disp[2] += (force * fVec[2]);
      }
    }
  }
};

//------------------------------------------------------------------------------
// For each point, build the connectivity array to nearby points. The number
// of neighbors is given by the specified neighborhood size.
template <typename PointsT>
struct BuildConnectivity
{
  PointsT* Points;
  int NeiSize;
  vtkAbstractPointLocator* Locator;
  vtkIdType* Conn;
  vtkSMPThreadLocalObject<vtkIdList> LocalNeighbors;

  BuildConnectivity(PointsT* pts, int neiSize, vtkAbstractPointLocator* loc, vtkIdType* conn)
    : Points(pts)
    , NeiSize(neiSize)
    , Locator(loc)
    , Conn(conn)
  {
  }

  void Initialize() { this->LocalNeighbors.Local()->Allocate(this->NeiSize + 1); }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    const auto tuples = vtk::DataArrayTupleRange<3>(this->Points, ptId, endPtId);
    vtkIdList* neis = this->LocalNeighbors.Local();
    double x[3];
    vtkIdType i, numInserted, *nptr;
    vtkIdType numNeis, *neighbors = this->Conn + (ptId * this->NeiSize);
    for (const auto tuple : tuples)
    {
      x[0] = static_cast<double>(tuple[0]);
      x[1] = static_cast<double>(tuple[1]);
      x[2] = static_cast<double>(tuple[2]);

      // Exclude ourselves from list of neighbors and be paranoid about it (that
      // is don't insert too many points)
      this->Locator->FindClosestNPoints(this->NeiSize + 1, x, neis);
      numNeis = neis->GetNumberOfIds();
      nptr = neis->GetPointer(0);
      for (numInserted = 0, i = 0; i < numNeis && numInserted < this->NeiSize; ++i)
      {
        if (*nptr != ptId)
        {
          neighbors[numInserted++] = *nptr;
        }
        ++nptr;
      }
      // In rare cases not all neighbors may be found, mark with a (-1)
      for (; numInserted < this->NeiSize; ++numInserted)
      {
        neighbors[numInserted] = (-1);
      }
      ++ptId; // move to the next point
      neighbors += this->NeiSize;
    }
  }

  // An Initialize() method requires a Reduce() method
  void Reduce() {}

}; // BuildConnectivity

// Hooks into dispatcher vtkArrayDispatch by providing a callable generic
struct ConnectivityWorker
{
  template <typename PointsT>
  void operator()(
    PointsT* pts, vtkIdType numPts, int neiSize, vtkAbstractPointLocator* loc, vtkIdType* conn)
  {
    BuildConnectivity<PointsT> buildConn(pts, neiSize, loc, conn);
    vtkSMPTools::For(0, numPts, buildConn);
  }
};

// Centralize the dispatch to avoid duplication
void UpdateConnectivity(
  vtkDataArray* pts, vtkIdType numPts, int neiSize, vtkAbstractPointLocator* loc, vtkIdType* conn)
{
  using vtkArrayDispatch::Reals;
  using ConnDispatch = vtkArrayDispatch::DispatchByValueType<Reals>;
  ConnectivityWorker connWorker;
  if (!ConnDispatch::Execute(pts, connWorker, numPts, neiSize, loc, conn))
  { // Fallback to slowpath for other point types
    connWorker(pts, numPts, neiSize, loc, conn);
  }
}

//------------------------------------------------------------------------------
// Constrain point movement depending on classification. The point can move
// freely, on a plane, or is fixed.
struct PointConstraints
{
  enum
  {
    UNCONSTRAINED = 0,
    PLANE = 1,
    CORNER = 2
  };
  vtkNew<vtkCharArray> ClassificationArray;
  vtkNew<vtkDoubleArray> NormalsArray;
  char* Classification;
  double* Normals;
  double FixedAngle;
  double BoundaryAngle;
  PointConstraints(vtkIdType numPts, double fa, double ba)
    : FixedAngle(fa)
    , BoundaryAngle(ba)
  {
    this->ClassificationArray->SetName("Constraint Scalars");
    this->ClassificationArray->SetNumberOfComponents(1);
    this->ClassificationArray->SetNumberOfTuples(numPts);
    this->Classification = this->ClassificationArray->GetPointer(0);
    this->NormalsArray->SetNumberOfComponents(3);
    this->NormalsArray->SetNumberOfTuples(numPts);
    this->Normals = this->NormalsArray->GetPointer(0);
  }
  vtkDataArray* GetClassificationArray() { return this->ClassificationArray; }
  vtkDataArray* GetNormalsArray() { return this->NormalsArray; }
};

// Characterize the mesh, including classifying points as to whether they
// are on boundaries or are fixed.
template <typename PointsT>
struct CharacterizeMesh
{
  PointsT* Points;
  int NeiSize;
  const vtkIdType* Conn;
  PointConstraints* Constraints;
  double MinLength;
  double MaxLength;
  double AverageLength;

  double CornerAngle;
  double BoundaryAngle;

  vtkSMPThreadLocal<double*> LocalNeiPoints;
  vtkSMPThreadLocal<double*> LocalNeiNormals;
  vtkSMPThreadLocal<double> LocalMin;
  vtkSMPThreadLocal<double> LocalMax;
  vtkSMPThreadLocal<vtkIdType> LocalNEdges;
  vtkSMPThreadLocal<double> LocalAve;

  CharacterizeMesh(PointsT* inPts, int neiSize, const vtkIdType* conn, PointConstraints* c)
    : Points(inPts)
    , NeiSize(neiSize)
    , Conn(conn)
    , Constraints(c)
    , MinLength(0.0)
    , MaxLength(0.0)
    , AverageLength(0.0)
  {
    if (this->Constraints)
    {
      this->CornerAngle = cos(vtkMath::RadiansFromDegrees(this->Constraints->FixedAngle));
      this->BoundaryAngle = cos(vtkMath::RadiansFromDegrees(this->Constraints->BoundaryAngle));
    }
  }

  void ClassifyPoint(vtkIdType ptId, double x[3], vtkIdType neiSize, const vtkIdType* neis,
    const double* neiPts, double* normals)
  {
    // Compute an average normal
    double* normal;
    double* aveN = this->Constraints->Normals + 3 * ptId;
    aveN[0] = aveN[1] = aveN[2] = 0.0;
    char* classification = this->Constraints->Classification + ptId;
    for (auto i = 0; i < neiSize; ++i)
    {
      if (neis[i] >= 0) // check for valid neighbor
      {
        normal = normals + 3 * i;
        normal[0] = neiPts[3 * i] - x[0];
        normal[1] = neiPts[3 * i + 1] - x[1];
        normal[2] = neiPts[3 * i + 2] - x[2];
        vtkMath::Normalize(normal);
        aveN[0] += normal[0];
        aveN[1] += normal[1];
        aveN[2] += normal[2];
      }
    }
    double mag = vtkMath::Normalize(aveN);
    if (mag == 0.0)
    {
      *classification = PointConstraints::UNCONSTRAINED;
      return;
    }

    // Now determine angles away from average normal. This provides
    // a classification.
    double dot, minDot = 1.0;
    for (auto i = 0; i < neiSize; ++i)
    {
      if (neis[i] >= 0)
      {
        normal = normals + 3 * i;
        dot = vtkMath::Dot(normal, aveN);
        minDot = (dot < minDot ? dot : minDot);
      }
    }
    if (minDot >= this->CornerAngle)
    {
      *classification = PointConstraints::CORNER;
    }
    else if (minDot >= this->BoundaryAngle)
    {
      *classification = PointConstraints::PLANE;
    }
    else
    {
      *classification = PointConstraints::UNCONSTRAINED;
    }
  }

  void Initialize()
  {
    this->LocalNeiPoints.Local() = new double[this->NeiSize * 3];
    this->LocalNeiNormals.Local() = new double[this->NeiSize * 3];
    this->LocalMin.Local() = VTK_DOUBLE_MAX;
    this->LocalMax.Local() = VTK_DOUBLE_MIN;
    this->LocalNEdges.Local() = 0;
    this->LocalAve.Local() = 0.0;
  }

  // Determine the minimum and maximum edge lengths
  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    const vtkIdType* neis = this->Conn + (this->NeiSize * ptId);
    const auto inPts = vtk::DataArrayTupleRange<3>(this->Points);
    double& min = this->LocalMin.Local();
    double& max = this->LocalMax.Local();
    vtkIdType& nEdges = this->LocalNEdges.Local();
    double& ave = this->LocalAve.Local();
    double* neiPts = this->LocalNeiPoints.Local();
    double* neiNormals = this->LocalNeiNormals.Local();
    double x[3], y[3], len;
    vtkIdType neiId;

    for (; ptId < endPtId; ++ptId, neis += this->NeiSize)
    {
      x[0] = inPts[ptId][0];
      x[1] = inPts[ptId][1];
      x[2] = inPts[ptId][2];
      // Gather the coordinates of the points surrounding the point to smooth
      for (auto i = 0; i < this->NeiSize; ++i)
      {
        neiId = neis[i];
        if (neiId >= 0) // valid connection to another point
        {
          neiPts[3 * i] = y[0] = inPts[neiId][0];
          neiPts[3 * i + 1] = y[1] = inPts[neiId][1];
          neiPts[3 * i + 2] = y[2] = inPts[neiId][2];

          // Process valid connections, and to reduce work only edges where
          // the neighbor id > pt id.
          if (neiId >= 0 && neiId > ptId)
          {
            len = sqrt(vtkMath::Distance2BetweenPoints(x, y));
            min = std::min(len, min);
            max = std::max(len, max);
            ++nEdges;
            ave += len;
          }
        }
      }
      // Classify point if requested
      if (this->Constraints)
      {
        this->ClassifyPoint(ptId, x, this->NeiSize, neis, neiPts, neiNormals);
      }
    } // for all points in this batch
  }   // operator()

  // Composite the data
  void Reduce()
  {
    // Don't need scratch storage anymore
    for (auto& iter : this->LocalNeiPoints)
    {
      delete[] iter;
    }
    for (auto& iter : this->LocalNeiNormals)
    {
      delete[] iter;
    }

    // Min / max edge lengths
    double min = VTK_DOUBLE_MAX;
    double max = VTK_DOUBLE_MIN;
    for (auto iter = this->LocalMin.begin(); iter != this->LocalMin.end(); ++iter)
    {
      min = std::min(*iter, min);
    }
    for (auto iter = this->LocalMax.begin(); iter != this->LocalMax.end(); ++iter)
    {
      max = std::max(*iter, max);
    }
    this->MinLength = min;
    this->MaxLength = max;

    // Average length
    vtkIdType numEdges = 0;
    double ave = 0.0;
    for (auto iter = this->LocalNEdges.begin(); iter != this->LocalNEdges.end(); ++iter)
    {
      numEdges += *iter;
    }
    for (auto iter = this->LocalAve.begin(); iter != this->LocalAve.end(); ++iter)
    {
      ave += *iter;
    }
    this->AverageLength = ave / static_cast<double>(numEdges);
  }
}; // CharacterizeMesh

// Hooks into dispatcher vtkArrayDispatch by providing a callable generic
struct MeshWorker
{
  double MinLength;
  double MaxLength;
  double AverageLength;
  MeshWorker()
    : MinLength(0.0)
    , MaxLength(0.0)
  {
  }

  template <typename PointsT>
  void operator()(
    PointsT* inPts, vtkIdType numPts, int neiSize, vtkIdType* conn, PointConstraints* constraints)
  {
    CharacterizeMesh<PointsT> characterize(inPts, neiSize, conn, constraints);
    vtkSMPTools::For(0, numPts, characterize);
    this->MinLength = characterize.MinLength;
    this->MaxLength = characterize.MaxLength;
    this->AverageLength = characterize.AverageLength;
  }
}; // MeshWorker

//------------------------------------------------------------------------------
// Smoothing operation based on double buffering (simplifies threading). In
// general the types of points (input and output buffers) can be different.
template <typename PointsT1, typename PointsT2>
struct SmoothPoints
{
  PointsT1* InPoints;
  PointsT2* OutPoints;
  int NeiSize;
  double MaximumStepSize;
  const vtkIdType* Conn;
  DisplacePoint* Displace;
  PointConstraints* Constraints;
  vtkPlane* Plane;
  double PlaneOrigin[3];
  double PlaneNormal[3];
  vtkSMPThreadLocal<double*> LocalNeiPoints;

  SmoothPoints(PointsT1* inPts, PointsT2* outPts, int neiSize, double maxStep,
    const vtkIdType* conn, DisplacePoint* f, PointConstraints* c, vtkPlane* plane)
    : InPoints(inPts)
    , OutPoints(outPts)
    , NeiSize(neiSize)
    , MaximumStepSize(maxStep)
    , Conn(conn)
    , Displace(f)
    , Constraints(c)
    , Plane(plane)
  {
    if (this->Plane)
    {
      this->Plane->GetOrigin(this->PlaneOrigin);
      this->Plane->GetNormal(this->PlaneNormal);
      vtkMath::Normalize(this->PlaneNormal);
    }
  }

  void Initialize() { this->LocalNeiPoints.Local() = new double[this->NeiSize * 3]; }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    const vtkIdType* neis = this->Conn + (this->NeiSize * ptId);
    const auto inPts = vtk::DataArrayTupleRange<3>(this->InPoints);
    auto outPts = vtk::DataArrayTupleRange<3>(this->OutPoints);
    double* neiPts = this->LocalNeiPoints.Local();
    vtkIdType neiId;
    double x[3], disp[3];

    for (; ptId < endPtId; ++ptId, neis += this->NeiSize)
    {
      // Gather the coordinates of the points surrounding the point to smooth
      for (auto i = 0; i < this->NeiSize; ++i)
      {
        neiId = neis[i];
        if (neiId >= 0) // valid connection to another point
        {
          neiPts[3 * i] = inPts[neiId][0];
          neiPts[3 * i + 1] = inPts[neiId][1];
          neiPts[3 * i + 2] = inPts[neiId][2];
        }
      } // neighborhood coordinates

      // Now compute a displacement for the current point in this neighborhood
      x[0] = inPts[ptId][0];
      x[1] = inPts[ptId][1];
      x[2] = inPts[ptId][2];
      (*this->Displace)(ptId, x, this->NeiSize, neis, neiPts, disp);

      // It may be necessary to constrain the points's motion
      if (this->Constraints)
      {
        // Check the classification of the point. May have to constrain it's motion.
        if (this->Constraints->Classification[ptId] == PointConstraints::CORNER)
        { // do nothing, point will never move
          disp[0] = disp[1] = disp[2] = 0.0;
        }
        else
        {
          if (this->Constraints->Classification[ptId] == PointConstraints::PLANE)
          { // constrain to a point constraint plane
            double* normal = this->Constraints->Normals + 3 * ptId;
            vtkPlane::ProjectVector(disp, x, normal, disp);
          }
        }
      } // if constraints

      // Control the size of the step
      double len = vtkMath::Norm(disp);
      if (len > 0.0 && len > this->MaximumStepSize)
      {
        disp[0] *= (this->MaximumStepSize / len);
        disp[1] *= (this->MaximumStepSize / len);
        disp[2] *= (this->MaximumStepSize / len);
      }

      // Move the point
      x[0] += disp[0];
      x[1] += disp[1];
      x[2] += disp[2];

      // If point motion is constrained to a plane, project onto the plane
      if (this->Plane)
      {
        vtkPlane::ProjectPoint(x, this->PlaneOrigin, this->PlaneNormal, x);
      }

      // Update the output points buffer
      outPts[ptId][0] = x[0];
      outPts[ptId][1] = x[1];
      outPts[ptId][2] = x[2];

    } // for all points in this batch
  }   // operator()

  void Reduce()
  {
    for (auto& iter : this->LocalNeiPoints)
    {
      delete[] iter;
    }
  }

}; // SmoothPoints

// Hooks into dispatcher vtkArrayDispatch by providing a callable generic
struct SmoothWorker
{
  template <typename PointsT1, typename PointsT2>
  void operator()(PointsT1* inPts, PointsT2* outPts, vtkIdType numPts, int neiSize,
    double maxStepSize, vtkIdType* conn, DisplacePoint* f, PointConstraints* c, vtkPlane* plane)
  {
    SmoothPoints<PointsT1, PointsT2> smooth(inPts, outPts, neiSize, maxStepSize, conn, f, c, plane);
    vtkSMPTools::For(0, numPts, smooth);
  }
}; // SmoothWorker

} // anonymous namespace

//================= Begin class proper =======================================
//------------------------------------------------------------------------------
vtkPointSmoothingFilter::vtkPointSmoothingFilter()
{
  this->NeighborhoodSize = 8; // works well for 2D
  this->SmoothingMode = DEFAULT_SMOOTHING;
  this->NumberOfIterations = 20;
  this->NumberOfSubIterations = 10;
  this->MaximumStepSize = 0.01;
  this->Convergence = 0.0; // runs to number of specified iterations
  this->FrameFieldArray = nullptr;

  this->Locator = vtkStaticPointLocator::New();

  this->EnableConstraints = false;
  this->FixedAngle = 60.0;
  this->BoundaryAngle = 110.0;
  this->GenerateConstraintScalars = false;
  this->GenerateConstraintNormals = false;

  this->ComputePackingRadius = true;
  this->PackingRadius = 1.0;
  this->PackingFactor = 1.0;
  this->AttractionFactor = 0.5;

  this->MotionConstraint = UNCONSTRAINED_MOTION;
  this->Plane = nullptr;
}

//------------------------------------------------------------------------------
vtkPointSmoothingFilter::~vtkPointSmoothingFilter()
{
  this->SetFrameFieldArray(nullptr);
  this->SetLocator(nullptr);
  this->SetPlane(nullptr);
}

//------------------------------------------------------------------------------
int vtkPointSmoothingFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the input and output datasets
  vtkSmartPointer<vtkPointSet> input = vtkPointSet::GetData(inputVector[0]);
  vtkPointSet* output = vtkPointSet::GetData(outputVector);

  // Copy the input to the output as a starting point. We'll replace
  // the points and update point data later on.
  output->CopyStructure(input);
  output->GetCellData()->PassData(input->GetCellData());
  if (this->NumberOfIterations <= 0) // Trivial case: 0 iterations
  {
    output->GetPointData()->PassData(input->GetPointData());
    return 1;
  }

  // Check the input
  vtkIdType numPts = input->GetNumberOfPoints();
  if (numPts < 1)
  {
    vtkLog(ERROR, "Points required");
    return 0;
  }
  if (!this->Locator)
  {
    vtkLog(ERROR, "Point locator required\n");
    return 0;
  }

  // Determine the smoothing mode
  vtkPointData *inPD = input->GetPointData(), *outPD = output->GetPointData();
  vtkDataArray* inScalars = inPD->GetScalars();
  vtkDataArray* inTensors = inPD->GetTensors();
  vtkDataArray* frameField = this->FrameFieldArray;
  int smoothingMode = UNIFORM_SMOOTHING;
  if (this->SmoothingMode == DEFAULT_SMOOTHING)
  {
    smoothingMode = (frameField != nullptr
        ? FRAME_FIELD_SMOOTHING
        : (inTensors != nullptr ? TENSOR_SMOOTHING
                                : (inScalars != nullptr ? SCALAR_SMOOTHING : UNIFORM_SMOOTHING)));
  }
  else if (this->SmoothingMode == GEOMETRIC_SMOOTHING)
  {
    smoothingMode = GEOMETRIC_SMOOTHING;
  }
  else if (this->SmoothingMode == SCALAR_SMOOTHING && inScalars != nullptr)
  {
    smoothingMode = SCALAR_SMOOTHING;
  }
  else if (this->SmoothingMode == TENSOR_SMOOTHING && inTensors != nullptr)
  {
    smoothingMode = TENSOR_SMOOTHING;
  }
  else if (this->SmoothingMode == FRAME_FIELD_SMOOTHING && frameField != nullptr)
  {
    smoothingMode = FRAME_FIELD_SMOOTHING;
  }
  vtkDebugMacro(<< "Smoothing glyphs: mode is: " << smoothingMode);

  // We'll build a locator for two purposes: 1) to build a point connectivity
  // list (connections to close points); and 2) interpolate data from neighbor
  // points.
  vtkDataArray* pts = input->GetPoints()->GetData();
  this->Locator->SetDataSet(input);
  this->Locator->BuildLocator();

  // The point neighborhood must be initially defined. Later on we'll update
  // it periodically.
  vtkIdType neiSize = (numPts < this->NeighborhoodSize ? numPts : this->NeighborhoodSize);
  vtkIdType* conn = new vtkIdType[numPts * neiSize];
  UpdateConnectivity(pts, numPts, neiSize, this->Locator, conn);

  // In order to perform smoothing properly we need to characterize the point
  // spacing and/or scalar, tensor, and or frame field data values. Later on
  // this enables the appropriate computation of the smoothing forces on the
  // points. Also classify the points as to on boundary or on edge etc. This
  // calculation is only done if not manually overridden.
  using vtkArrayDispatch::Reals;
  double radius = this->PackingRadius;
  PointConstraints* constraints = nullptr;
  if (this->EnableConstraints || this->ComputePackingRadius)
  {
    if (this->EnableConstraints)
    {
      constraints = new PointConstraints(numPts, this->FixedAngle, this->BoundaryAngle);
    }
    using MeshDispatch = vtkArrayDispatch::DispatchByValueType<Reals>;
    MeshWorker meshWorker;
    if (!MeshDispatch::Execute(pts, meshWorker, numPts, neiSize, conn, constraints))
    { // Fallback to slowpath for other point types
      meshWorker(pts, numPts, neiSize, conn, constraints);
    }
    this->PackingRadius = radius = meshWorker.AverageLength / 2.0;
  }

  // Establish the type of inter-point forces/displacements
  vtkSmartPointer<vtkDataArray> computedFrameField;
  DisplacePoint* disp;
  if (smoothingMode == UNIFORM_SMOOTHING)
  {
    disp = new UniformDisplacement(nullptr, radius, this->PackingFactor, this->AttractionFactor);
  }
  else if (smoothingMode == SCALAR_SMOOTHING)
  {
    double range[2];
    inPD->GetRange(inScalars->GetName(), range);
    disp =
      new ScalarDisplacement(inScalars, radius, this->PackingFactor, this->AttractionFactor, range);
  }
  else if (smoothingMode == TENSOR_SMOOTHING || smoothingMode == FRAME_FIELD_SMOOTHING)
  {
    double detRange[2];
    computedFrameField.TakeReference(smoothingMode == TENSOR_SMOOTHING
        ? ComputeEigenvalues(inTensors)
        : PadFrameField(this->FrameFieldArray));
    CharacterizeTensor(computedFrameField, numPts, detRange);
    disp = new TensorDisplacement(
      computedFrameField, radius, this->PackingFactor, this->AttractionFactor, detRange);
  }
  else // GEOMETRIC_SMOOTHING
  {
    disp = new GeometricDisplacement(nullptr, radius, this->PackingFactor, this->AttractionFactor);
  }

  // Prepare for smoothing. We double buffer the points. The output points
  // type is the same as the input points type.
  vtkPoints* pts0 = vtkPoints::New();
  pts0->SetDataType(pts->GetDataType());
  pts0->SetNumberOfPoints(numPts);
  pts0->DeepCopy(input->GetPoints());
  vtkPoints* pts1 = vtkPoints::New();
  pts1->SetDataType(pts->GetDataType());
  pts1->SetNumberOfPoints(numPts);
  vtkPoints *swapBuf, *inBuf = pts0, *outBuf = pts1;
  int numSubIters =
    (this->NumberOfSubIterations < this->NumberOfIterations ? this->NumberOfSubIterations
                                                            : this->NumberOfIterations);
  double maxStepSize = this->MaximumStepSize;
  vtkPlane* plane = (this->MotionConstraint == PLANE_MOTION && this->Plane ? this->Plane : nullptr);

  // We need to incrementally compute a local neighborhood. This will be
  // performed every sub-iterations. This requires another point locator to
  // periodically rebuild the neighborhood connectivity. The initial point locator
  // is not modified we we can interpolate from the original points.
  vtkPolyData* tmpPolyData = vtkPolyData::New();
  tmpPolyData->SetPoints(inBuf);
  vtkAbstractPointLocator* tmpLocator = this->Locator->NewInstance();
  tmpLocator->SetDataSet(tmpPolyData);

  // Begin looping. We dispatch to various workers depending on points type.
  using SmoothDispatch = vtkArrayDispatch::Dispatch2ByValueType<Reals, Reals>;
  SmoothWorker sworker;
  bool converged = false;
  for (int iterNum = 0; iterNum < this->NumberOfIterations && !converged; ++iterNum)
  {
    // Perform a smoothing iteration using the current connectivity.
    if (!SmoothDispatch::Execute(inBuf->GetData(), outBuf->GetData(), sworker, numPts, neiSize,
          maxStepSize, conn, disp, constraints, plane))
    { // Fallback to slowpath for other point types
      sworker(inBuf->GetData(), outBuf->GetData(), numPts, neiSize, maxStepSize, conn, disp,
        constraints, plane);
    }

    // Build connectivity every sub-iterations.
    if (!(iterNum % numSubIters))
    {
      // Build the point connectivity list as necessary. This is threaded and optimized over
      // Real types.
      tmpLocator->BuildLocator();
      UpdateConnectivity(pts, numPts, neiSize, tmpLocator, conn);
    }

    swapBuf = inBuf;
    inBuf = outBuf;
    outBuf = swapBuf;
    tmpLocator->Modified(); // ensure a rebuild the next time we build connectivity
  }                         // over all iterations

  // Set the output points
  output->SetPoints(outBuf);

  // If constraint scalars are requested, produce them
  if (constraints && this->GenerateConstraintScalars)
  {
    outPD->AddArray(constraints->GetClassificationArray());
  }

  // If constraint vectors are requested, produce them
  if (constraints && this->GenerateConstraintNormals)
  {
    outPD->AddArray(constraints->GetNormalsArray());
  }

  // Clean up
  delete disp;
  delete constraints;
  delete[] conn;
  pts0->Delete();
  pts1->Delete();
  tmpPolyData->Delete();
  tmpLocator->Delete();

  // Copy point data
  outPD->PassData(inPD);

  return 1;
}

//------------------------------------------------------------------------------
int vtkPointSmoothingFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtkPointSmoothingFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Smoothing Mode: " << this->SmoothingMode << endl;
  os << indent << "Neighborhood Size: " << this->NeighborhoodSize << endl;
  os << indent << "Number of Iterations: " << this->NumberOfIterations << endl;
  os << indent << "Number of Sub-iterations: " << this->NumberOfSubIterations << endl;
  os << indent << "Maximum Step Size: " << this->MaximumStepSize << endl;
  os << indent << "Convergence: " << this->Convergence << endl;
  os << indent << "Frame Field Array: " << this->FrameFieldArray << "\n";
  os << indent << "Locator: " << this->Locator << "\n";

  os << indent << "Enable Constraints: " << (this->EnableConstraints ? "On\n" : "Off\n");
  os << indent << "Fixed Angle: " << this->FixedAngle << "\n";
  os << indent << "Boundary Angle: " << this->BoundaryAngle << "\n";
  os << indent
     << "Generate Constraint Scalars: " << (this->GenerateConstraintScalars ? "On\n" : "Off\n");
  os << indent
     << "Generate Constraint Normals: " << (this->GenerateConstraintNormals ? "On\n" : "Off\n");

  os << indent << "Compute Packing Radius: " << (this->ComputePackingRadius ? "On\n" : "Off\n");
  os << indent << "Packing Radius: " << this->PackingRadius << "\n";
  os << indent << "Packing Factor: " << this->PackingFactor << "\n";
  os << indent << "Attraction Factor: " << this->AttractionFactor << "\n";

  os << indent << "Motion Constraint: " << this->MotionConstraint << "\n";
  os << indent << "Plane: " << this->Plane << "\n";
}
VTK_ABI_NAMESPACE_END
