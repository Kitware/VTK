/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphericalPointIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSphericalPointIterator.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkUnsignedIntArray.h"

#include <numeric>
#include <vector>

vtkStandardNewMacro(vtkSphericalPointIterator);

//=============================================================================
// Define the internal structure of the spherical point iterator below.

// An axis of the spherical point iterator
struct Axis
{
  double A[3];
  Axis(double a[3])
    : A{ a[0], a[1], a[2] }
  {
  }
};

// Represent the iterator
struct vtkSphericalPointIterator::SphericalPointIterator
{
  // The points being referred to
  vtkDataSet* DataSet;

  // The center point of the iterator
  double Center[3];

  // *Unit* normals defining the axes
  std::vector<Axis> Axes;

  // The points (referred to by id) along each axis. Typically sorted,
  // but not necessarily (depending on user specification).
  std::vector<std::vector<vtkIdType>> Points;

  // Represent the current iteration state. The iteration state consists of:
  // the current axis number, and the current point index on that
  // axis. NumVisited number keeps track of the number of points processed
  // at the current state of iteration.
  int CurrentAxis;         // during traversal, the current axis
  int CurrentPointIndex;   // during the traversal, the current
  vtkIdType NumVisited;    // The number of points visited so far
  vtkIdType MaxPointIndex; // the maximum number of points projected on any one axis
  vtkIdType NumPts;        // Total number of points in the neighborhood

  // Determine whether the axis and point index specified contain valid
  // information.
  bool IsValid(int axis, int ptIdx)
  {
    if (axis >= static_cast<int>(this->Axes.size()))
    {
      return false;
    }
    std::vector<vtkIdType>& axisPts = this->Points[axis];
    if (ptIdx >= static_cast<int>(axisPts.size()))
    {
      return false;
    }

    return true;
  }

  // Clear out the iterator data to an empty state.
  void Clear()
  {
    this->DataSet = nullptr;
    this->Axes.clear();
    this->Points.clear();
    this->CurrentAxis = 0;
    this->CurrentPointIndex = 0;
    this->NumVisited = 0;
    this->NumPts = 0;
  }

  // Propagate the VTK class information to this internal iterator.
  // Make sure the axes are normalized. There is an upper limit on
  // the number of axes, here it's set to a ridiculously large number.
  // Typically the number of axes is 20 or less.
  void Define(vtkDataSet* ds, vtkDoubleArray* axes)
  {
    constexpr int MAX_NUM_AXES = 100000;

    this->Clear();
    vtkIdType numAxes = axes->GetNumberOfTuples();
    numAxes = (numAxes < MAX_NUM_AXES ? numAxes : MAX_NUM_AXES);
    this->Points.resize(numAxes); // creates empty points vectors

    this->DataSet = ds;
    double a[3];
    for (auto i = 0; i < numAxes; ++i)
    {
      axes->GetTuple(i, a);
      vtkMath::Normalize(a);
      this->Axes.emplace_back(a);
    }
  }

  // Reset (empty out) the points lists.
  void Reset()
  {
    for (auto& pts : this->Points)
    {
      pts.clear();
    }
  }

  // Return the number of axes
  vtkIdType GetNumberOfAxes() { return static_cast<vtkIdType>(this->Axes.size()); }

  struct RadialTuple
  {
    vtkIdType PtId;
    double R2;
    RadialTuple(vtkIdType ptId, double r2)
      : PtId(ptId)
      , R2(r2)
    {
    }
    bool operator<(const RadialTuple& tuple) const { return (this->R2 < tuple.R2); }
    bool operator>(const RadialTuple& tuple) const { return (this->R2 > tuple.R2); }
  };

  // Radially sort the points on the axes specified. Will sort in
  // either an ascending or descending direction.
  void SortPointsOnAxis(std::vector<vtkIdType>& points, int dir)
  {
    // Make sure there is work to do
    if (points.empty())
    {
      return;
    }

    // Sort a tuple based on distance**2
    std::vector<struct RadialTuple> radialSort;
    radialSort.reserve(points.size());
    double x[3], d2;
    for (auto ptId : points)
    {
      this->DataSet->GetPoint(ptId, x);
      d2 = vtkMath::Distance2BetweenPoints(x, this->Center);
      radialSort.emplace_back(RadialTuple(ptId, d2));
    }
    if (dir == vtkSphericalPointIterator::SORT_DESCENDING)
    {
      std::sort(radialSort.begin(), radialSort.end(), std::greater<RadialTuple>());
    }
    else // ascending
    {
      std::sort(radialSort.begin(), radialSort.end());
    }

    // Update the ordering of the points along the axis
    vtkIdType sze = static_cast<vtkIdType>(points.size());
    for (vtkIdType i = 0; i < sze; ++i)
    {
      points[i] = radialSort[i].PtId;
    }
  }

  // Initialize the traversal process. Specify whether sorting along the axes is required.
  bool Initialize(double center[3], vtkIdType numNei, vtkIdType* neighborhood, int sort)
  {
    // Reset the points lists.
    this->Reset();
    this->NumPts = 0;

    // Redefine the center of iteration
    this->Center[0] = center[0];
    this->Center[1] = center[1];
    this->Center[2] = center[2];

    // Project points onto the best axis (with maximum positive dot product)
    double x[3], v[3], dp, dpMax;
    int axis, axisMax = 0;
    for (auto i = 0; i < numNei; ++i)
    {
      vtkIdType ptId = neighborhood[i];
      this->DataSet->GetPoint(ptId, x);
      v[0] = x[0] - this->Center[0];
      v[1] = x[1] - this->Center[1];
      v[2] = x[2] - this->Center[2];
      dpMax = 0; // angle between vectors must be <90 degrees
      int sze = static_cast<int>(this->Axes.size());
      for (axis = 0; axis < sze; ++axis)
      {
        double* a = this->Axes[axis].A;
        dp = vtkMath::Dot(a, v);
        if (dp > dpMax)
        {
          dpMax = dp;
          axisMax = axis;
        }
      } // for all axes
      if (dpMax > 0)
      {
        this->Points[axisMax].emplace_back(ptId);
        this->NumPts++;
      }
    } // for all points in neighborhood

    // If sorting is requested, then do the extra work
    // of sorting along each of the axes.
    if (sort != vtkSphericalPointIterator::SORT_NONE)
    {
      int sze = static_cast<int>(this->Axes.size());
      for (axis = 0; axis < sze; ++axis)
      {
        this->SortPointsOnAxis(this->Points[axis], sort);
      }
    }

    // Determine the maximum number of points on any axis.
    this->MaxPointIndex = 0;
    int sze = static_cast<int>(this->Axes.size());
    for (axis = 0; axis < sze; ++axis)
    {
      vtkIdType npts = (vtkIdType)this->Points[axis].size();
      this->MaxPointIndex = (npts > this->MaxPointIndex ? npts : this->MaxPointIndex);
    }

    return true;
  }

  // Begin forward iteration. The complexity of forward iteration is that the
  // number of points associated with each axes varies (and may be zero).
  // The iteration process begins with axis0, point0, and then moves onto
  // axis1, point0, and so on. Axes and/or points may have to be skipped
  // until all points are iterated over.
  void GoToFirstPoint()
  {
    this->CurrentPointIndex = 0;
    int sze = static_cast<int>(this->Axes.size());
    for (this->CurrentAxis = 0; this->CurrentAxis < sze; ++this->CurrentAxis)
    {
      if (this->IsValid(this->CurrentAxis, this->CurrentPointIndex))
      {
        this->NumVisited = 1;
        return;
      }
    } // over all axes
  }

  // Determine whether formard iteration is complete.
  bool IsDoneWithTraversal() { return (this->NumVisited <= this->NumPts ? false : true); }

  // Go to the the next point during forward iteration.
  void GoToNextPoint()
  {
    // Spiral around the axes, incrementing point index when
    // all axes have been visited once.
    do
    {
      if (++this->CurrentAxis >= static_cast<int>(this->Axes.size()))
      {
        this->CurrentAxis = 0;
        this->CurrentPointIndex++;
      }
    } while (this->CurrentPointIndex < this->MaxPointIndex &&
      !this->IsValid(this->CurrentAxis, this->CurrentPointIndex));

    this->NumVisited++;
  }

  // During forward iteration, retrieve the current point id and its
  // coordinates.
  void GetCurrentPoint(vtkIdType& ptId, double x[3])
  {
    ptId = (this->Points[this->CurrentAxis])[this->CurrentPointIndex];
    this->DataSet->GetPoint(ptId, x);
  }

  // During forward iteration, retrieve the current point id.
  vtkIdType GetCurrentPoint() { return (this->Points[this->CurrentAxis])[this->CurrentPointIndex]; }

  // Randomly access a point from the iterator. Returns a
  // point id, or <0 if no such point exists.
  vtkIdType GetPoint(int axis, int ptIdx)
  {
    if (!this->IsValid(axis, ptIdx))
    {
      return -1;
    }

    std::vector<vtkIdType>& axisPts = this->Points[axis];
    return axisPts[ptIdx];
  }

  // Randomly access a point from the iterator. Returns a
  // point id, or <0 if no such point exists. Also returns
  // the point coordinate x[3].
  vtkIdType GetPoint(int axis, int ptIdx, double x[3])
  {
    vtkIdType ptId = this->GetPoint(axis, ptIdx);
    if (ptId < 0)
    {
      return -1;
    }
    else
    {
      this->DataSet->GetPoint(ptId, x);
      return ptId;
    }
  }

  // Get the points along a particular axis.
  void GetAxisPoints(int axis, vtkIdType& npts, const vtkIdType*& pts)
  {
    if (axis < static_cast<int>(this->Axes.size()))
    {
      npts = this->Points[axis].size();
      pts = this->Points[axis].data();
    }
    else
    {
      npts = 0;
      pts = nullptr;
    }
  }

}; // vtkSphericalPointIterator::SphericalPointIterator

//==============================================================================
// Begin VTK class proper

//------------------------------------------------------------------------------
vtkSphericalPointIterator::vtkSphericalPointIterator()
  : Iterator(new vtkSphericalPointIterator::SphericalPointIterator())
{
  // Smart pointers are constructed with nullptr
  this->Sorting = SORT_NONE;
}

//------------------------------------------------------------------------------
void vtkSphericalPointIterator::SetAxes(int axesType, int resolution)
{
  vtkNew<vtkDoubleArray> axes;
  axes->SetNumberOfComponents(3);
  int res = (resolution < 1 ? 1 : resolution);

  if (axesType == XY_CW_AXES)
  {
    axes->SetNumberOfTuples(res);
    for (auto i = res; i > 0; --i)
    {
      double theta = ((static_cast<double>(i) / res) * 2.0 * vtkMath::Pi());
      axes->SetTuple3(res - i, cos(theta), sin(theta), 0);
    }
  }

  else if (axesType == XY_CCW_AXES)
  {
    axes->SetNumberOfTuples(res);
    for (auto i = 0; i < res; ++i)
    {
      double theta = ((static_cast<double>(i) / res) * 2.0 * vtkMath::Pi());
      axes->SetTuple3(i, cos(theta), sin(theta), 0);
    }
  }

  else if (axesType == XY_SQUARE_AXES)
  {
    axes->SetNumberOfTuples(4);
    axes->SetTuple3(0, -1, 0, 0);
    axes->SetTuple3(1, 1, 0, 0);
    axes->SetTuple3(2, 0, -1, 0);
    axes->SetTuple3(3, 0, 1, 0);
  }

  else if (axesType == CUBE_AXES)
  {
    axes->SetNumberOfTuples(6);
    axes->SetTuple3(0, -1, 0, 0);
    axes->SetTuple3(1, 1, 0, 0);
    axes->SetTuple3(2, 0, -1, 0);
    axes->SetTuple3(3, 0, 1, 0);
    axes->SetTuple3(4, 0, 0, -1);
    axes->SetTuple3(5, 0, 0, 1);
  }

  else if (axesType == OCTAHEDRON_AXES)
  {
    axes->SetNumberOfTuples(8);
    axes->SetTuple3(0, 0, -0.47140451272, -0.33333333333);
    axes->SetTuple3(1, 0.47140451272, 0, -0.33333333333);
    axes->SetTuple3(2, 0, 0.47140451272, -0.33333333333);
    axes->SetTuple3(3, -0.47140451272, 0, -0.33333333333);
    axes->SetTuple3(4, 0, -0.47140451272, 0.33333333333);
    axes->SetTuple3(5, 0.47140451272, 0, 0.33333333333);
    axes->SetTuple3(6, 0, 0.47140451272, 0.33333333333);
    axes->SetTuple3(7, -0.47140451272, 0, 0.33333333333);
  }

  else if (axesType == CUBE_OCTAHEDRON_AXES)
  {
    axes->SetNumberOfTuples(14);
    axes->SetTuple3(0, -1, 0, 0);
    axes->SetTuple3(1, 1, 0, 0);
    axes->SetTuple3(2, 0, -1, 0);
    axes->SetTuple3(3, 0, 1, 0);
    axes->SetTuple3(4, 0, 0, -1);
    axes->SetTuple3(5, 0, 0, 1);
    axes->SetTuple3(6, 1, 1, 1);
    axes->SetTuple3(7, -1, 1, 1);
    axes->SetTuple3(8, 1, -1, 1);
    axes->SetTuple3(9, -1, -1, 1);
    axes->SetTuple3(10, 1, 1, -1);
    axes->SetTuple3(11, -1, 1, -1);
    axes->SetTuple3(12, 1, -1, -1);
    axes->SetTuple3(13, -1, -1, -1);
  }

  else if (axesType == DODECAHEDRON_AXES)
  {
    axes->SetNumberOfTuples(12);
    axes->SetTuple3(0, -0.055132041737, 0.43301268705, 0.66655578242);
    axes->SetTuple3(1, 0.055132041737, -0.43301268705, 0.66655578242);
    axes->SetTuple3(2, -0.055132041737, -0.43301268705, -0.66655578242);
    axes->SetTuple3(3, 0.055132041737, 0.43301268705, -0.66655578242);
    axes->SetTuple3(4, 0.46708616567, 0.64549721701, 0);
    axes->SetTuple3(5, -0.46708616567, 0.64549721701, 0);
    axes->SetTuple3(6, -0.46708616567, -0.64549721701, 0);
    axes->SetTuple3(7, 0.46708616567, -0.64549721701, 0);
    axes->SetTuple3(8, 0.66655578242, -0.055132041737, 0.43301268705);
    axes->SetTuple3(9, 0.66655578242, 0.055132041737, -0.43301268705);
    axes->SetTuple3(10, -0.66655578242, -0.055132041737, -0.43301268705);
    axes->SetTuple3(11, -0.66655578242, 0.055132041737, 0.43301268705);
  }

  else // icosahedron
  {
    axes->SetNumberOfTuples(20);
    axes->SetTuple3(0, 0, 0.74234422048, -0.28355026245);
    axes->SetTuple3(1, 0, 0.74234422048, 0.28355026245);
    axes->SetTuple3(2, -0.28355026245, 0, 0.74234422048);
    axes->SetTuple3(3, 0.28355026245, 0, 0.74234422048);
    axes->SetTuple3(4, 0.28355026245, 0, -0.74234422048);
    axes->SetTuple3(5, -0.28355026245, 0, -0.74234422048);
    axes->SetTuple3(6, 0, -0.74234422048, 0.28355026245);
    axes->SetTuple3(7, 0, -0.74234422048, -0.28355026245);
    axes->SetTuple3(8, -0.74234422048, 0.28355026245, 0);
    axes->SetTuple3(9, -0.74234422048, -0.28355026245, 0);
    axes->SetTuple3(10, 0.74234422048, 0.28355026245, 0);
    axes->SetTuple3(11, 0.74234422048, -0.28355026245, 0);
    axes->SetTuple3(12, -0.45879395803, 0.45879395803, 0.45879395803);
    axes->SetTuple3(13, 0.45879395803, 0.45879395803, 0.45879395803);
    axes->SetTuple3(14, -0.45879395803, 0.45879395803, -0.45879395803);
    axes->SetTuple3(15, 0.45879395803, 0.45879395803, -0.45879395803);
    axes->SetTuple3(16, -0.45879395803, -0.45879395803, -0.45879395803);
    axes->SetTuple3(17, 0.45879395803, -0.45879395803, -0.45879395803);
    axes->SetTuple3(18, -0.45879395803, -0.45879395803, 0.45879395803);
    axes->SetTuple3(19, 0.45879395803, -0.45879395803, 0.45879395803);
  }

  this->SetAxes(axes);
}

//------------------------------------------------------------------------------
bool vtkSphericalPointIterator::Initialize(
  double center[3], vtkIdType numNei, vtkIdType* neighborhood)
{
  // Check input
  if (!this->DataSet || !this->Axes)
  {
    return false;
  }

  if (this->BuildTime < this->MTime)
  {
    // The first time requires defining the iterator
    this->Iterator->Define(this->DataSet, this->Axes);
    this->BuildTime.Modified();
  }

  return this->Iterator->Initialize(center, numNei, neighborhood, this->Sorting);
}

//------------------------------------------------------------------------------
bool vtkSphericalPointIterator::Initialize(double center[3], vtkIdList* neighborhood)
{
  return this->Initialize(center, neighborhood->GetNumberOfIds(), neighborhood->GetPointer(0));
}

//------------------------------------------------------------------------------
// Iterator over all points in a dataset
bool vtkSphericalPointIterator::Initialize(double center[3])
{
  if (!this->DataSet)
  {
    return false;
  }
  vtkIdType numPts = this->DataSet->GetNumberOfPoints();
  std::vector<vtkIdType> ptMap(numPts);
  std::iota(ptMap.begin(), ptMap.end(), 0);

  return this->Initialize(center, numPts, ptMap.data());
}

//------------------------------------------------------------------------------
void vtkSphericalPointIterator::GoToFirstPoint()
{
  this->Iterator->GoToFirstPoint();
}

//------------------------------------------------------------------------------
bool vtkSphericalPointIterator::IsDoneWithTraversal()
{
  return this->Iterator->IsDoneWithTraversal();
}

//------------------------------------------------------------------------------
void vtkSphericalPointIterator::GoToNextPoint()
{
  this->Iterator->GoToNextPoint();
}

//------------------------------------------------------------------------------
void vtkSphericalPointIterator::GetCurrentPoint(vtkIdType& ptId, double x[3])
{
  this->Iterator->GetCurrentPoint(ptId, x);
}

//------------------------------------------------------------------------------
vtkIdType vtkSphericalPointIterator::GetCurrentPoint()
{
  return this->Iterator->GetCurrentPoint();
}

//------------------------------------------------------------------------------
vtkIdType vtkSphericalPointIterator::GetPoint(int axis, int ptIdx)
{
  return this->Iterator->GetPoint(axis, ptIdx);
}

//------------------------------------------------------------------------------
void vtkSphericalPointIterator::GetAxisPoints(int axis, vtkIdType& npts, const vtkIdType*& pts)
{
  return this->Iterator->GetAxisPoints(axis, npts, pts);
}

//------------------------------------------------------------------------------
vtkIdType vtkSphericalPointIterator::GetNumberOfAxes()
{
  return this->Iterator->GetNumberOfAxes();
}

//------------------------------------------------------------------------------
// The Initialize() method is assumed to have been called.
void vtkSphericalPointIterator::BuildRepresentation(vtkPolyData* pd)
{
  // Initialize the representation
  pd->Reset();

  // Get the basic iterator information
  int numAxes = this->GetNumberOfAxes();
  double* center = this->Iterator->Center;

  // Build the polydata
  vtkNew<vtkPoints> pts;
  pts->SetDataTypeToDouble();
  pts->SetNumberOfPoints(numAxes + 1);
  vtkNew<vtkCellArray> lines;
  vtkNew<vtkUnsignedIntArray> lineNumbers;
  lineNumbers->SetNumberOfTuples(numAxes);
  pd->SetPoints(pts);
  pd->SetLines(lines);
  pd->GetCellData()->AddArray(lineNumbers);

  // Loop over axes. The center point goes first.
  vtkIdType linePts[2];
  linePts[0] = 0;
  pts->SetPoint(0, center[0], center[1], center[2]);
  for (auto i = 1; i <= numAxes; ++i)
  {
    int axisNum = i - 1;
    double x[3];
    x[0] = center[0] + this->Iterator->Axes[axisNum].A[0];
    x[1] = center[1] + this->Iterator->Axes[axisNum].A[1];
    x[2] = center[2] + this->Iterator->Axes[axisNum].A[2];
    pts->SetPoint(i, x);
    linePts[1] = i;
    lines->InsertNextCell(2, linePts);
    lineNumbers->SetTypedComponent(axisNum, 0, axisNum);
  }
}

//------------------------------------------------------------------------------
void vtkSphericalPointIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "DataSet: " << this->DataSet << "\n";
  os << indent << "Number of Axes: " << this->Axes->GetNumberOfTuples() << "\n";
  os << indent << "Axes: " << this->Axes << "\n";
  os << indent << "Sorting: " << this->Sorting << "\n";
}
