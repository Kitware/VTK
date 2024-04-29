// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPCANormalEstimation.h"

#include "vtkAbstractPointLocator.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPCANormalEstimation);
vtkCxxSetObjectMacro(vtkPCANormalEstimation, Locator, vtkAbstractPointLocator);

namespace
{
namespace Utils
{
//------------------------------------------------------------------------------
///@{
/**
 * Find the closest points to a given point according to the behavior set
 * by SearchMode. If SearchMode is set to KNN, K points (set by SampleSize)
 * are selected regardless of their location, if Radius is also set to a
 * value, the code checks if the farthest point found (K-th) is inside this
 * radius. In that case, the surrounding points are reselected inside this
 * radius. If SearchMode is set to Radius, the surrounding points are
 * selected inside the radius, if SampleSize is also set to a value, the
 * code checks if at least SampleSize (K) points have been selected.
 * Otherwise, SampleSize (K) points are reselected.
 */
template <typename T>
void FindPoints(vtkAbstractPointLocator* locator, T* inPts, double x[3], int searchMode,
  int sampleSize, double radius, vtkIdList* ids)
{
  switch (searchMode)
  {
    case vtkPCANormalEstimation::RADIUS:
    {
      locator->FindPointsWithinRadius(radius, x, ids);
      // If not enough points are found, then use K nearest neighbors
      if (ids->GetNumberOfIds() < sampleSize)
      {
        locator->FindClosestNPoints(sampleSize, x, ids);
      }
      break;
    }
    case vtkPCANormalEstimation::KNN:
    {
      locator->FindClosestNPoints(sampleSize, x, ids);
      // Retrieve the farthest point found
      double farthestPoint[3];
      const T* point = inPts + 3 * ids->GetId(ids->GetNumberOfIds() - 1);
      for (int i = 0; i < 3; ++i)
      {
        farthestPoint[i] = static_cast<double>(*point++);
      }

      double squaredDistance = vtkMath::Distance2BetweenPoints(x, farthestPoint);
      // If the points found are too densely packed, then use radius search
      if (squaredDistance < radius * radius)
      {
        locator->FindPointsWithinRadius(radius, x, ids);
      }
      break;
    }
  }
}
} // Utils namespace

//------------------------------------------------------------------------------
// The threaded core of the algorithm.
template <typename T>
struct GenerateNormals
{
  const T* Points;
  vtkAbstractPointLocator* Locator;
  int SampleSize;
  float Radius;
  float* Normals;
  int SearchMode;
  int Orient;
  double OPoint[3];
  bool Flip;

  // Don't want to allocate working arrays on every thread invocation. Thread local
  // storage lots of new/delete.
  vtkSMPThreadLocalObject<vtkIdList> PIds;

  GenerateNormals(T* points, vtkAbstractPointLocator* loc, int sample, double radius,
    float* normals, int searchMode, int orient, double opoint[3], bool flip)
    : Points(points)
    , Locator(loc)
    , SampleSize(sample)
    , Radius(radius)
    , Normals(normals)
    , SearchMode(searchMode)
    , Orient(orient)
    , Flip(flip)
  {
    this->OPoint[0] = opoint[0];
    this->OPoint[1] = opoint[1];
    this->OPoint[2] = opoint[2];
  }

  // Just allocate a little bit of memory to get started.
  void Initialize()
  {
    vtkIdList*& pIds = this->PIds.Local();
    pIds->Allocate(128); // allocate some memory
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    const T* px = this->Points + 3 * ptId;
    const T* py;
    float* n = this->Normals + 3 * ptId;
    double x[3], mean[3], o[3];
    vtkIdList*& pIds = this->PIds.Local();
    vtkIdType numPts, nei;
    int sample, i;
    double *a[3], a0[3], a1[3], a2[3], xp[3];
    a[0] = a0;
    a[1] = a1;
    a[2] = a2;
    double *v[3], v0[3], v1[3], v2[3];
    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
    double eVecMin[3], eVal[3];
    float flipVal = (this->Flip ? -1.0 : 1.0);

    for (; ptId < endPtId; ++ptId)
    {
      x[0] = static_cast<double>(*px++);
      x[1] = static_cast<double>(*px++);
      x[2] = static_cast<double>(*px++);

      // Retrieve the local neighborhood
      Utils::FindPoints(
        this->Locator, this->Points, x, this->SearchMode, this->SampleSize, this->Radius, pIds);

      numPts = pIds->GetNumberOfIds();

      // First step: compute the mean position of the neighborhood.
      mean[0] = mean[1] = mean[2] = 0.0;
      for (sample = 0; sample < numPts; ++sample)
      {
        nei = pIds->GetId(sample);
        py = this->Points + 3 * nei;
        mean[0] += static_cast<double>(*py++);
        mean[1] += static_cast<double>(*py++);
        mean[2] += static_cast<double>(*py);
      }
      mean[0] /= static_cast<double>(numPts);
      mean[1] /= static_cast<double>(numPts);
      mean[2] /= static_cast<double>(numPts);

      // Now compute the covariance matrix
      a0[0] = a1[0] = a2[0] = 0.0;
      a0[1] = a1[1] = a2[1] = 0.0;
      a0[2] = a1[2] = a2[2] = 0.0;
      for (sample = 0; sample < numPts; ++sample)
      {
        nei = pIds->GetId(sample);
        py = this->Points + 3 * nei;
        xp[0] = static_cast<double>(*py++) - mean[0];
        xp[1] = static_cast<double>(*py++) - mean[1];
        xp[2] = static_cast<double>(*py) - mean[2];
        for (i = 0; i < 3; i++)
        {
          a0[i] += xp[0] * xp[i];
          a1[i] += xp[1] * xp[i];
          a2[i] += xp[2] * xp[i];
        }
      }
      for (i = 0; i < 3; i++)
      {
        a0[i] /= static_cast<double>(numPts);
        a1[i] /= static_cast<double>(numPts);
        a2[i] /= static_cast<double>(numPts);
      }

      // Next extract the eigenvectors and values
      vtkMath::Jacobi(a, eVal, v);
      // eVecMax[0] = v[0][0]; eVecMax[1] = v[1][0]; eVecMax[2] = v[2][0];
      // eVecMid[0] = v[0][1]; eVecMid[1] = v[1][1]; eVecMid[2] = v[2][1];
      eVecMin[0] = v[0][2];
      eVecMin[1] = v[1][2];
      eVecMin[2] = v[2][2];

      // Orient properly
      if (this->Orient == vtkPCANormalEstimation::POINT)
      {
        o[0] = this->OPoint[0] - x[0];
        o[1] = this->OPoint[1] - x[1];
        o[2] = this->OPoint[2] - x[2];
        if (vtkMath::Dot(o, eVecMin) < 0.0)
        {
          eVecMin[0] *= -1;
          eVecMin[1] *= -1;
          eVecMin[2] *= -1;
        }
      }

      // Finally compute the point normal (which is the smallest eigenvector)
      *n++ = flipVal * eVecMin[0];
      *n++ = flipVal * eVecMin[1];
      *n++ = flipVal * eVecMin[2];

    } // for all points
  }

  void Reduce() {}

  static void Execute(vtkPCANormalEstimation* self, vtkIdType numPts, T* points, float* normals,
    int searchMode, int orient, double opoint[3], bool flip)
  {
    GenerateNormals gen(points, self->GetLocator(), self->GetSampleSize(), self->GetRadius(),
      normals, searchMode, orient, opoint, flip);
    vtkSMPTools::For(0, numPts, gen);
  }
}; // GenerateNormals
} // anonymous namespace

//================= Begin VTK class proper =======================================
//------------------------------------------------------------------------------
vtkPCANormalEstimation::~vtkPCANormalEstimation()
{
  this->SetLocator(nullptr);
}

//------------------------------------------------------------------------------
// Produce the output data
int vtkPCANormalEstimation::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet* input = vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Check the input
  if (!input || !output)
  {
    return 1;
  }
  vtkIdType numPts = input->GetNumberOfPoints();
  if (numPts < 1)
  {
    return 1;
  }

  // Start by building the locator.
  if (!this->Locator)
  {
    vtkErrorMacro(<< "Point locator required\n");
    return 0;
  }
  this->Locator->SetDataSet(input);
  this->Locator->BuildLocator();

  // Generate the point normals.
  vtkFloatArray* normals = vtkFloatArray::New();
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(numPts);
  normals->SetName("PCANormals");
  float* n = static_cast<float*>(normals->GetVoidPointer(0));

  void* inPtr = input->GetPoints()->GetVoidPointer(0);
  switch (input->GetPoints()->GetDataType())
  {
    vtkTemplateMacro(GenerateNormals<VTK_TT>::Execute(this, numPts, (VTK_TT*)inPtr, n,
      this->SearchMode, this->NormalOrientation, this->OrientationPoint, this->FlipNormals));
  }

  // Orient the normals in a consistent fashion (if requested). This requires a traversal
  // across the point cloud, traversing neighbors that are in close proximity.
  if (this->NormalOrientation == vtkPCANormalEstimation::GRAPH_TRAVERSAL)
  {
    vtkIdType ptId;
    char* pointMap = new char[numPts];
    std::fill_n(pointMap, numPts, static_cast<char>(0));
    vtkIdList* wave = vtkIdList::New();
    wave->Allocate(numPts / 4 + 1, numPts);
    vtkIdList* wave2 = vtkIdList::New();
    wave2->Allocate(numPts / 4 + 1, numPts);

    for (ptId = 0; ptId < numPts; ptId++)
    {
      if (pointMap[ptId] == 0)
      {
        wave->InsertNextId(ptId); // begin next connected wave
        pointMap[ptId] = 1;
        this->TraverseAndFlip(input->GetPoints(), n, pointMap, wave, wave2);
        wave->Reset();
        wave2->Reset();
      }
    } // for all points
    delete[] pointMap;
    wave->Delete();
    wave2->Delete();
  } // if graph traversal required

  // Now send the normals to the output and clean up
  output->SetPoints(input->GetPoints());
  output->GetPointData()->PassData(input->GetPointData());
  output->GetPointData()->SetNormals(normals);
  normals->Delete();

  return 1;
}

//------------------------------------------------------------------------------
// Mark current point as visited and assign cluster number.  Note:
// traversal occurs across proximally located points.
//
void vtkPCANormalEstimation::TraverseAndFlip(
  vtkPoints* inPts, float* normals, char* pointMap, vtkIdList* wave, vtkIdList* wave2)
{
  vtkIdType i, j, numPts, numIds, ptId;
  vtkIdList* tmpWave;
  double x[3];
  float *n, *n2;
  vtkIdList* neighborPointIds = vtkIdList::New();

  while ((numIds = wave->GetNumberOfIds()) > 0)
  {
    for (i = 0; i < numIds; i++) // for all points in this wave
    {
      ptId = wave->GetId(i);

      void* inPtr = inPts->GetVoidPointer(0);
      switch (inPts->GetDataType())
      {
        // Use template macro to use raw data of the points for all data types
        vtkTemplateMacro({
          const VTK_TT* points = (VTK_TT*)inPtr;
          for (int l = 0; l < 3; ++l)
          {
            x[l] = static_cast<double>(points[3 * ptId + l]);
          }
          // Select neighboring points according to the SearchMode
          ::Utils::FindPoints(this->Locator, points, x, this->SearchMode, this->SampleSize,
            this->Radius, neighborPointIds);
        });
      }

      n = normals + 3 * ptId;

      numPts = neighborPointIds->GetNumberOfIds();
      for (j = 0; j < numPts; ++j)
      {
        ptId = neighborPointIds->GetId(j);
        if (pointMap[ptId] == 0)
        {
          pointMap[ptId] = 1;
          n2 = normals + 3 * ptId;
          if (vtkMath::Dot(n, n2) < 0.0)
          {
            *n2++ *= -1;
            *n2++ *= -1;
            *n2 *= -1;
          }
          wave2->InsertNextId(ptId);
        } // if point not yet visited
      }   // for all neighbors
    }     // for all cells in this wave

    tmpWave = wave;
    wave = wave2;
    wave2 = tmpWave;
    tmpWave->Reset();
  } // while wave is not empty

  neighborPointIds->Delete();
}

//------------------------------------------------------------------------------
int vtkPCANormalEstimation::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtkPCANormalEstimation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Sample Size: " << this->SampleSize << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Search Mode: " << this->SearchMode << endl;
  os << indent << "Normal Orientation: " << this->NormalOrientation << endl;
  os << indent << "Orientation Point: (" << this->OrientationPoint[0] << ","
     << this->OrientationPoint[1] << "," << this->OrientationPoint[2] << ")\n";
  os << indent << "Flip Normals: " << (this->FlipNormals ? "On\n" : "Off\n");
  os << indent << "Locator: " << this->Locator << "\n";
}
VTK_ABI_NAMESPACE_END
