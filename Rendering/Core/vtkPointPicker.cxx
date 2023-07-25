// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPointPicker.h"

#include "vtkAbstractVolumeMapper.h"
#include "vtkBox.h"
#include "vtkCellArray.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkImageMapper3D.h"
#include "vtkMapper.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkProp3D.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPointPicker);

//------------------------------------------------------------------------------
// Accelerated methods for picking points
namespace
{ // anonymous

bool UpdateClosestPoint(double x[3], const double p1[3], double ray[3], double rayFactor,
  double tol, double& tMin, double& distMin)
{
  double t =
    (ray[0] * (x[0] - p1[0]) + ray[1] * (x[1] - p1[1]) + ray[2] * (x[2] - p1[2])) / rayFactor;

  // If we find a point closer than we currently have, see whether it
  // lies within the pick tolerance and clipping planes. We keep track
  // of the point closest to the line (use a fudge factor for points
  // nearly the same distance away.)

  if (t < 0. || t > 1. || t > (tMin + tol))
  {
    return false;
  }

  double maxDist = 0.0;
  double projXYZ[3];
  for (int i = 0; i < 3; i++)
  {
    projXYZ[i] = p1[i] + t * ray[i];
    double dist = fabs(x[i] - projXYZ[i]);
    if (dist > maxDist)
    {
      maxDist = dist;
    }
  }

  if (maxDist <= tol && maxDist < distMin)
  {
    distMin = maxDist;
    tMin = t;
    return true;
  }
  return false;
}

// Threaded dataset picking
struct PickPoints
{
  vtkDataSet* DataSet;
  double P1[3];
  double Ray[3];
  double RayFactor;
  double Tol;

  vtkIdType MinPtId;
  double MinT;
  double MinDist;
  double MinXYZ[3];

  struct vtkLocalData
  {
    vtkIdType MinPtId;
    double MinT;
    double MinDist;
    double MinXYZ[3];
    vtkLocalData()
      : MinPtId(-1)
      , MinT(VTK_DOUBLE_MAX)
      , MinDist(VTK_DOUBLE_MAX)
    {
    }
  };
  vtkSMPThreadLocal<vtkLocalData> LocalData;

  PickPoints(vtkDataSet* ds, const double p1[3], double ray[3], double rayFactor, double tol)
    : DataSet(ds)
    , RayFactor(rayFactor)
    , Tol(tol)
    , MinPtId(-1)
    , MinT(VTK_DOUBLE_MAX)
    , MinDist(VTK_DOUBLE_MAX)
  {
    this->P1[0] = p1[0];
    this->P1[1] = p1[1];
    this->P1[2] = p1[2];
    this->Ray[0] = ray[0];
    this->Ray[1] = ray[1];
    this->Ray[2] = ray[2];
  }

  void Initialize()
  {
    this->LocalData.Local().MinPtId = -1;
    this->LocalData.Local().MinT = VTK_DOUBLE_MAX;
    this->LocalData.Local().MinDist = VTK_DOUBLE_MAX;
    this->LocalData.Local().MinXYZ[0] = 0.0;
    this->LocalData.Local().MinXYZ[1] = 0.0;
    this->LocalData.Local().MinXYZ[2] = 0.0;
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    double x[3];
    auto& localData = this->LocalData.Local();
    vtkIdType& minPtId = localData.MinPtId;
    double& tMin = localData.MinT;
    double& minPtDist = localData.MinDist;
    double* minXYZ = localData.MinXYZ;

    for (; ptId < endPtId; ++ptId)
    {
      this->DataSet->GetPoint(ptId, x);
      if (UpdateClosestPoint(x, this->P1, this->Ray, this->RayFactor, this->Tol, tMin, minPtDist))
      {
        minPtId = ptId;
        minXYZ[0] = x[0];
        minXYZ[1] = x[1];
        minXYZ[2] = x[2];
      }
    }
  }

  // Composite the selected point
  void Reduce()
  {
    for (auto iter = this->LocalData.begin(); iter != this->LocalData.end(); ++iter)
    {
      if ((*iter).MinT < this->MinT)
      {
        this->MinPtId = (*iter).MinPtId;
        this->MinT = (*iter).MinT;
        this->MinDist = (*iter).MinDist;
        this->MinXYZ[0] = (*iter).MinXYZ[0];
        this->MinXYZ[1] = (*iter).MinXYZ[1];
        this->MinXYZ[2] = (*iter).MinXYZ[2];
      }
    }
  }

  static vtkIdType Execute(vtkIdType numPts, vtkDataSet* ds, const double p1[3], double ray[3],
    double rayFactor, double tol, double& tMin, double minXYZ[3])
  {
    PickPoints pick(ds, p1, ray, rayFactor, tol);
    vtkSMPTools::For(0, numPts, pick);
    tMin = pick.MinT;
    minXYZ[0] = pick.MinXYZ[0];
    minXYZ[1] = pick.MinXYZ[1];
    minXYZ[2] = pick.MinXYZ[2];
    return pick.MinPtId;
  }
};

} // anonymous namespace

//------------------------------------------------------------------------------
inline vtkCellArray* GET_CELLS(int cell_type, vtkPolyData* poly_input)
{
  switch (cell_type)
  {
    case 0:
      return poly_input->GetVerts();
    case 1:
      return poly_input->GetLines();
    case 2:
      return poly_input->GetPolys();
    case 3:
      return poly_input->GetStrips();
  }
  return nullptr;
}

//------------------------------------------------------------------------------
vtkPointPicker::vtkPointPicker()
{
  this->PointId = -1;
  this->UseCells = 0;
}

//------------------------------------------------------------------------------
double vtkPointPicker::IntersectWithLine(const double p1[3], const double p2[3], double tol,
  vtkAssemblyPath* path, vtkProp3D* p, vtkAbstractMapper3D* m)
{
  vtkIdType minPtId = -1;
  double tMin = VTK_DOUBLE_MAX;
  double minXYZ[3];
  vtkDataSet* input;
  vtkMapper* mapper;
  vtkAbstractVolumeMapper* volumeMapper = nullptr;
  vtkImageMapper3D* imageMapper = nullptr;

  double ray[3], rayFactor;
  if (!vtkPicker::CalculateRay(p1, p2, ray, rayFactor))
  {
    vtkDebugMacro("Zero length ray");
    return 2.0;
  }

  // Get the underlying dataset.
  //
  if ((mapper = vtkMapper::SafeDownCast(m)) != nullptr)
  {
    input = mapper->GetInput();
  }
  else if ((volumeMapper = vtkAbstractVolumeMapper::SafeDownCast(m)) != nullptr)
  {
    input = volumeMapper->GetDataSetInput();
  }
  else if ((imageMapper = vtkImageMapper3D::SafeDownCast(m)) != nullptr)
  {
    input = imageMapper->GetInput();
  }
  else
  {
    return 2.0;
  }

  //   For image, find the single intersection point
  //
  if (imageMapper != nullptr)
  {
    if (input->GetNumberOfPoints() == 0)
    {
      vtkDebugMacro("No points in input");
      return 2.0;
    }

    // Get the slice plane for the image and intersect with ray
    double normal[4];
    imageMapper->GetSlicePlaneInDataCoords(p->GetMatrix(), normal);
    double w1 = vtkMath::Dot(p1, normal) + normal[3];
    double w2 = vtkMath::Dot(p2, normal) + normal[3];
    if (w1 * w2 >= 0)
    {
      w1 = 0.0;
      w2 = 1.0;
    }
    double w = (w2 - w1);
    double x[3];
    x[0] = (p1[0] * w2 - p2[0] * w1) / w;
    x[1] = (p1[1] * w2 - p2[1] * w1) / w;
    x[2] = (p1[2] * w2 - p2[2] * w1) / w;

    // Get the one point that will be checked
    minPtId = input->FindPoint(x);
    if (minPtId > -1)
    {
      input->GetPoint(minPtId, minXYZ);
      double distMin = VTK_DOUBLE_MAX;
      UpdateClosestPoint(minXYZ, p1, ray, rayFactor, tol, tMin, distMin);

      //  Now compare this against other actors.
      //
      if (tMin < this->GlobalTMin)
      {
        this->MarkPicked(path, p, m, tMin, minXYZ);
        this->PointId = minPtId;
        vtkDebugMacro("Picked point id= " << minPtId);
      }
    }
  }
  else if (input)
  {
    //  Project each point onto ray.  Keep track of the one within the
    //  tolerance and closest to the eye (and within the clipping range).
    //
    minPtId = this->IntersectDataSetWithLine(p1, ray, rayFactor, tol, input, tMin, minXYZ);

    //  Now compare this against other actors.
    //
    if (minPtId > -1 && tMin < this->GlobalTMin)
    {
      this->MarkPicked(path, p, m, tMin, minXYZ);
      this->PointId = minPtId;
      vtkDebugMacro("Picked point id= " << minPtId);
    }
  }
  else if (mapper != nullptr)
  {
    // a mapper mapping composite dataset input returns a nullptr vtkDataSet.
    // Iterate over all leaf datasets and find the closest point in any of
    // the leaf data sets
    vtkCompositeDataSet* composite =
      vtkCompositeDataSet::SafeDownCast(mapper->GetInputDataObject(0, 0));
    if (composite)
    {
      vtkIdType flatIndex = -1;
      vtkSmartPointer<vtkCompositeDataIterator> iter;
      iter.TakeReference(composite->NewIterator());
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
        if (!ds)
        {
          vtkDebugMacro(<< "Skipping " << iter->GetCurrentDataObject()->GetClassName()
                        << " block at index " << iter->GetCurrentFlatIndex());
          continue;
        }

        // First check if the bounding box of the data set is hit.
        double bounds[6];
        ds->GetBounds(bounds);
        bounds[0] -= tol;
        bounds[1] += tol;
        bounds[2] -= tol;
        bounds[3] += tol;
        bounds[4] -= tol;
        bounds[5] += tol;
        double tDummy;
        double xyzDummy[3];

        // only intersect dataset if bounding box is hit
        if (vtkBox::IntersectBox(bounds, p1, ray, xyzDummy, tDummy))
        {
          vtkIdType ptId =
            this->IntersectDataSetWithLine(p1, ray, rayFactor, tol, ds, tMin, minXYZ);
          if (ptId > -1)
          {
            input = ds;
            minPtId = ptId;
            flatIndex = iter->GetCurrentFlatIndex();
          }
        }
      }
      if (minPtId > -1 && tMin < this->GlobalTMin)
      {
        this->MarkPickedData(path, tMin, minXYZ, mapper, input, flatIndex);
        this->PointId = minPtId;
        vtkDebugMacro("Picked point id= " << minPtId << " in block " << flatIndex);
      }
    }
  }
  return tMin;
}

//------------------------------------------------------------------------------
vtkIdType vtkPointPicker::IntersectDataSetWithLine(const double p1[3], double ray[3],
  double rayFactor, double tol, vtkDataSet* dataSet, double& tMin, double minXYZ[3])
{
  if (dataSet->GetNumberOfPoints() == 0)
  {
    vtkDebugMacro("No points in input");
    return 2.0;
  }
  vtkIdType minPtId = -1;
  vtkPolyData* poly_input = vtkPolyData::SafeDownCast(dataSet);

  if (this->UseCells && (poly_input != nullptr))
  {
    double minPtDist = VTK_DOUBLE_MAX;

    for (int iCellType = 0; iCellType < 4; iCellType++)
    {
      vtkCellArray* cells = GET_CELLS(iCellType, poly_input);
      if (cells != nullptr)
      {
        cells->InitTraversal();
        vtkIdType n_cell_pts = 0;
        const vtkIdType* pt_ids = nullptr;
        while (cells->GetNextCell(n_cell_pts, pt_ids))
        {
          for (vtkIdType ptIndex = 0; ptIndex < n_cell_pts; ptIndex++)
          {
            vtkIdType ptId = pt_ids[ptIndex];
            double x[3];
            dataSet->GetPoint(ptId, x);

            if (UpdateClosestPoint(x, p1, ray, rayFactor, tol, tMin, minPtDist))
            {
              minPtId = ptId;
              minXYZ[0] = x[0];
              minXYZ[1] = x[1];
              minXYZ[2] = x[2];
            }
          }
        }
      }
    }
  }
  else // fallback to generic dataset
  {
    // Depending on the number of points, different approaches to intersecting the
    // points are used. For small number of points, a linear visit to each point is
    // used. For larger numbers, threading and/or locators may be used. The cutoff
    // thresholds for the scale of the work is arbitrary.
    vtkIdType numPts = dataSet->GetNumberOfPoints();
    if (numPts < 1000) // small number of points, just visit them all
    {
      double minPtDist = VTK_DOUBLE_MAX;

      for (vtkIdType ptId = 0; ptId < numPts; ptId++)
      {
        double x[3];
        dataSet->GetPoint(ptId, x);
        if (UpdateClosestPoint(x, p1, ray, rayFactor, tol, tMin, minPtDist))
        {
          minPtId = ptId;
          minXYZ[0] = x[0];
          minXYZ[1] = x[1];
          minXYZ[2] = x[2];
        }
      }
    }
    else // medium scale, threaded picking operation
    {
      minPtId = PickPoints::Execute(numPts, dataSet, p1, ray, rayFactor, tol, tMin, minXYZ);
    }
    // TODO: Use static point locator for huge number of points
  }

  return minPtId;
}

//------------------------------------------------------------------------------
void vtkPointPicker::Initialize()
{
  this->PointId = (-1);
  this->vtkPicker::Initialize();
}

//------------------------------------------------------------------------------
void vtkPointPicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Point Id: " << this->PointId << "\n";
}
VTK_ABI_NAMESPACE_END
