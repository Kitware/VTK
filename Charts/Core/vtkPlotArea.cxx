/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotArea.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPlotArea.h"

#include "vtkArrayDispatch.h"
#include "vtkAxis.h"
#include "vtkBoundingBox.h"
#include "vtkBrush.h"
#include "vtkCharArray.h"
#include "vtkContext2D.h"
#include "vtkContextMapper2D.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPoints2D.h"
#include "vtkTable.h"
#include "vtkWeakPointer.h"
#include "vtkVectorOperators.h"

#include <vector>
#include <set>
#include <cmath>
#include <algorithm>

namespace
{
  inline bool vtkIsBadPoint(const vtkVector2f& vec)
  {
    return (vtkMath::IsNan(vec.GetX()) || vtkMath::IsInf(vec.GetX()) ||
      vtkMath::IsNan(vec.GetY()) || vtkMath::IsInf(vec.GetY()));
  }
} // end of namespace

// Keeps all data-dependent meta-data that's updated in
// vtkPlotArea::Update.
class vtkPlotArea::vtkTableCache
{
  // PIMPL for STL vector...
  struct vtkIndexedVector2f
  {
    size_t index;
    vtkVector2f pos;
    static bool compVector3fX(
      const vtkIndexedVector2f& v1, const vtkIndexedVector2f& v2)
    {
      if (v1.pos.GetX() < v2.pos.GetX())
      {
        return true;
      }
      else
      {
        return false;
      }
    }
    // See if the point is within tolerance.
    static bool inRange(const vtkVector2f& point, const vtkVector2f& tol,
      const vtkVector2f& current)
    {
      if (current.GetX() > point.GetX() - tol.GetX() && current.GetX() < point.GetX() + tol.GetX() &&
        current.GetY() > point.GetY() - tol.GetY() && current.GetY() < point.GetY() + tol.GetY())
      {
        return true;
      }
      else
      {
        return false;
      }
    }
  };

  // DataStructure used to store sorted points.
  class VectorPIMPL : public std::vector<vtkIndexedVector2f>
  {
public:
  void Initialize(vtkVector2f* array, size_t n)
  {
    this->reserve(n);
    for (size_t i = 0; i < n; ++i)
    {
      vtkIndexedVector2f tmp;
      tmp.index = i;
      tmp.pos = array[i];
      this->push_back(tmp);
    }
  }

  //-----------------------------------------------------------------------------
  vtkIdType GetNearestPoint(
    const vtkVector2f& point, const vtkVector2f& tol, vtkVector2f* location)
  {
    // Set up our search array, use the STL lower_bound algorithm
    VectorPIMPL::iterator low;
    VectorPIMPL &v = *this;

    // Get the lowest point we might hit within the supplied tolerance
    vtkIndexedVector2f lowPoint;
    lowPoint.index = 0;
    lowPoint.pos = vtkVector2f(point.GetX()-tol.GetX(), 0.0f);
    low = std::lower_bound(v.begin(), v.end(), lowPoint, vtkIndexedVector2f::compVector3fX);

    // Now consider the y axis
    float highX = point.GetX() + tol.GetX();
    while (low != v.end())
    {
      if (vtkIndexedVector2f::inRange(point, tol, (*low).pos))
      {
        *location = (*low).pos;
        return static_cast<int>((*low).index);
      }
      else if (low->pos.GetX() > highX)
      {
        break;
      }
      ++low;
    }
    return -1;
  }
  };
private:
  vtkTimeStamp DataMTime;
  vtkTimeStamp BoundsMTime;

  // Unscaled data bounds.
  vtkBoundingBox DataBounds;

  vtkRectd ShiftScale;

  vtkTuple<double, 2> GetDataRange(vtkDataArray* array)
  {
    assert(array);

    if (this->ValidPointMask)
    {
      assert(array->GetNumberOfTuples() == this->ValidPointMask->GetNumberOfTuples());
      assert(array->GetNumberOfComponents() == this->ValidPointMask->GetNumberOfComponents());

      vtkArrayDispatch::Dispatch2ByArray<
          vtkArrayDispatch::Arrays, // First array is input, can be anything.
          vtkTypeList_Create_1(vtkCharArray) // Second is always vtkCharArray.
          > dispatcher;
      ComputeArrayRange worker;

      if (!dispatcher.Execute(array, this->ValidPointMask, worker))
      {
        vtkGenericWarningMacro(
              <<"Error computing range. Unsupported array type: "
              << array->GetClassName()
              << " (" << array->GetDataTypeAsString() << ").");
      }

      return worker.Result;
    }
    else
    {
      vtkArrayDispatch::Dispatch dispatcher;
      ComputeArrayRange worker;

      if (!dispatcher.Execute(array, worker))
      {
        vtkGenericWarningMacro(
              <<"Error computing range. Unsupported array type: "
              << array->GetClassName()
              << " (" << array->GetDataTypeAsString() << ").");
      }

      return worker.Result;
    }

  }

  struct ComputeArrayRange
  {
    vtkTuple<double, 2> Result;

    ComputeArrayRange()
    {
      Result[0] = VTK_DOUBLE_MAX;
      Result[1] = VTK_DOUBLE_MIN;
    }

    // Use mask:
    template <class ArrayT>
    void operator()(ArrayT *array, vtkCharArray* mask)
    {
      vtkIdType numTuples = array->GetNumberOfTuples();
      int numComps = array->GetNumberOfComponents();

      for (vtkIdType tupleIdx = 0; tupleIdx < numTuples; ++tupleIdx)
      {
        for (int compIdx = 0; compIdx < numComps; ++compIdx)
        {
          if (mask->GetTypedComponent(tupleIdx, compIdx) != 0)
          {
            typename ArrayT::ValueType val =
                array->GetTypedComponent(tupleIdx, compIdx);
            Result[0] = std::min(Result[0], static_cast<double>(val));
            Result[1] = std::max(Result[1], static_cast<double>(val));
          }
        }
      }
    }

    // No mask:
    template <class ArrayT>
    void operator()(ArrayT *array)
    {
      vtkIdType numTuples = array->GetNumberOfTuples();
      int numComps = array->GetNumberOfComponents();

      for (vtkIdType tupleIdx = 0; tupleIdx < numTuples; ++tupleIdx)
      {
        for (int compIdx = 0; compIdx < numComps; ++compIdx)
        {
          typename ArrayT::ValueType val =
              array->GetTypedComponent(tupleIdx, compIdx);
          Result[0] = std::min(Result[0], static_cast<double>(val));
          Result[1] = std::max(Result[1], static_cast<double>(val));
        }
      }
    }

  };

  struct CopyToPoints
  {
    float *Data;
    int DataIncrement;
    vtkIdType NumValues;
    vtkVector2d Transform;
    bool UseLog;

    CopyToPoints(float *data, int data_increment, vtkIdType numValues,
                 const vtkVector2d &ss, bool useLog)
      : Data(data),
        DataIncrement(data_increment),
        NumValues(numValues),
        Transform(ss),
        UseLog(useLog)
    {}

    CopyToPoints& operator=(const CopyToPoints &) VTK_DELETE_FUNCTION;

    // Use input array:
    template <class ArrayT>
    void operator()(ArrayT *array)
    {
    if (this->UseLog)
    {
      float *data = this->Data;
      for (vtkIdType valIdx = 0; valIdx < this->NumValues;
           ++valIdx, data += this->DataIncrement)
      {
        *data = log10(static_cast<float>(
                        (array->GetValue(valIdx) + this->Transform[0])
                        * this->Transform[1]));
      }
    }
    else
    {
      float *data = this->Data;
      for (vtkIdType valIdx = 0; valIdx < this->NumValues;
           ++valIdx, data += this->DataIncrement)
      {
        *data = static_cast<float>(
              (array->GetValue(valIdx) + this->Transform[0])
              * this->Transform[1]);
      }
    }
    }

    // No array, just iterate to number of values
    void operator()()
    {
    if (this->UseLog)
    {
      float *data = this->Data;
      for (vtkIdType valIdx = 0; valIdx < this->NumValues;
           ++valIdx, data += this->DataIncrement)
      {
        *data = log10(static_cast<float>((valIdx + this->Transform[0])
                                         * this->Transform[1]));
      }
    }
    else
    {
      float *data = this->Data;
      for (vtkIdType valIdx = 0; valIdx < this->NumValues;
           ++valIdx, data += this->DataIncrement)
      {
        *data = static_cast<float>((valIdx + this->Transform[0])
                                   * this->Transform[1]);
      }
    }
    }
  };

  VectorPIMPL SortedPoints;
public:
  // Array which marks valid points in the array. If NULL (the default), all
  // points in the input array are considered valid.
  vtkWeakPointer<vtkCharArray> ValidPointMask;

  // References to input arrays.
  vtkTuple<vtkWeakPointer<vtkDataArray>, 3> InputArrays;

  // Array for the points. These maintain the points that form the QuadStrip for
  // the area plot.
  vtkNew<vtkPoints2D> Points;

  // Set of point ids that are invalid or masked out.
  std::vector<vtkIdType> BadPoints;


  vtkTableCache()
  {
    this->Reset();
  }

  void Reset()
  {
    this->ValidPointMask = NULL;
    this->Points->Initialize();
    this->Points->SetDataTypeToFloat();
    this->BadPoints.clear();
  }

  bool IsInputDataValid() const
  {
    return this->InputArrays[1] != NULL && this->InputArrays[2] != NULL;
  }

  bool SetPoints(vtkDataArray* x, vtkDataArray* y1, vtkDataArray* y2)
  {
    if (y1 == NULL || y2 == NULL)
    {
      return false;
    }

    vtkIdType numTuples = y1->GetNumberOfTuples();

    // sanity check.
    assert((x == NULL || x->GetNumberOfTuples() == numTuples) && y2->GetNumberOfTuples() == numTuples);

    this->InputArrays[0] = x;
    this->InputArrays[1] = y1;
    this->InputArrays[2] = y2;
    this->Points->SetNumberOfPoints(numTuples * 2);
    this->SortedPoints.clear();
    this->DataMTime.Modified();
    return true;
  }

  void GetDataBounds(double bounds[4])
  {
    if (this->DataMTime > this->BoundsMTime)
    {
      vtkTuple<double, 2> rangeX, rangeY1, rangeY2;
      if (this->InputArrays[0])
      {
        rangeX = this->GetDataRange(this->InputArrays[0]);
      }
      else
      {
        rangeX[0] = 0; rangeX[1] = (this->Points->GetNumberOfPoints()/2-1);
      }
      rangeY1 = this->GetDataRange(this->InputArrays[1]);
      rangeY2 = this->GetDataRange(this->InputArrays[2]);

      this->DataBounds.Reset();
      this->DataBounds.SetMinPoint(rangeX[0], std::min(rangeY1[0], rangeY2[0]), 0);
      this->DataBounds.SetMaxPoint(rangeX[1], std::max(rangeY1[1], rangeY2[1]), 0);
      this->BoundsMTime.Modified();
    }
    double bds[6];
    this->DataBounds.GetBounds(bds);
    std::copy(bds, bds+4, bounds);
  }

  void UpdateCache(vtkPlotArea* self)
  {
    const vtkRectd& ss = self->GetShiftScale();
    vtkAxis* xaxis = self->GetXAxis();
    vtkAxis* yaxis = self->GetYAxis();

    if (this->Points->GetMTime()> this->DataMTime &&
      this->Points->GetMTime() > xaxis->GetMTime() &&
      this->Points->GetMTime() > yaxis->GetMTime() &&
      ss == this->ShiftScale)
    {
      // nothing to do.
      return;
    }

    vtkTuple<bool, 2> useLog;
    useLog[0] = xaxis->GetLogScaleActive();
    useLog[1] = yaxis->GetLogScaleActive();

    vtkIdType numTuples = this->InputArrays[1]->GetNumberOfTuples();
    assert(this->Points->GetNumberOfPoints() == 2*numTuples);

    float* data = reinterpret_cast<float*>(this->Points->GetVoidPointer(0));
    if (this->InputArrays[0])
    {
      vtkDataArray *array = this->InputArrays[0].GetPointer();
      vtkIdType numValues = array->GetNumberOfTuples() *
                            array->GetNumberOfComponents();

      CopyToPoints worker1(data, 4, numValues, vtkVector2d(ss[0], ss[2]),
                           useLog[0]);
      CopyToPoints worker2(&data[2], 4, numValues, vtkVector2d(ss[0], ss[2]),
                           useLog[0]);

      vtkArrayDispatch::Dispatch dispatcher;

      if (!dispatcher.Execute(array, worker1) ||
          !dispatcher.Execute(array, worker2))
      {
        vtkGenericWarningMacro("Error creating points, unsupported array type: "
                               << array->GetClassName()
                               << " (" << array->GetDataTypeAsString() << ").");
      }
    }
    else
    {
      CopyToPoints worker1(data, 4, numTuples, vtkVector2d(ss[0], ss[2]),
                           useLog[0]);
      CopyToPoints worker2(&data[2], 4, numTuples, vtkVector2d(ss[0], ss[2]),
                           useLog[0]);
      // No array, no need to dispatch:
      worker1();
      worker2();
    }

    vtkDataArray *array1 = this->InputArrays[1].GetPointer();
    vtkDataArray *array2 = this->InputArrays[2].GetPointer();
    vtkIdType numValues1 = array1->GetNumberOfTuples() *
                           array1->GetNumberOfComponents();
    vtkIdType numValues2 = array2->GetNumberOfTuples() *
                           array2->GetNumberOfComponents();

    CopyToPoints worker1(&data[1], 4, numValues1, vtkVector2d(ss[1], ss[3]),
                         useLog[1]);
    CopyToPoints worker2(&data[3], 4, numValues2, vtkVector2d(ss[1], ss[3]),
                         useLog[1]);

    vtkArrayDispatch::Dispatch dispatcher;

    if (!dispatcher.Execute(array1, worker1) ||
        !dispatcher.Execute(array2, worker2))
    {
      vtkGenericWarningMacro("Error creating points: Array dispatch failed.");
    }

    // Set the bad-points mask.
    vtkVector2f* vec2f = reinterpret_cast<vtkVector2f*>(this->Points->GetVoidPointer(0));
    for (vtkIdType cc=0; cc < numTuples; cc++)
    {
      bool is_bad = (this->ValidPointMask && this->ValidPointMask->GetValue(cc) == 0);
      is_bad =  is_bad || vtkIsBadPoint(vec2f[2*cc]);
      is_bad =  is_bad || vtkIsBadPoint(vec2f[2*cc + 1]);
      if (is_bad)
      {
        // this ensures that the GetNearestPoint() fails for masked out points.
        vec2f[2*cc] = vtkVector2f(vtkMath::Nan(), vtkMath::Nan());
        vec2f[2*cc + 1] = vtkVector2f(vtkMath::Nan(), vtkMath::Nan());
        this->BadPoints.push_back(cc);
      }
    }

    this->ShiftScale = ss;
    this->Points->Modified();
    this->SortedPoints.clear();
  }

  vtkIdType GetNearestPoint(
    const vtkVector2f& point, const vtkVector2f& tol, vtkVector2f* location)
  {
    if (this->Points->GetNumberOfPoints() == 0)
    {
      return -1;
    }

    if (this->SortedPoints.size() == 0)
    {
      float* data = reinterpret_cast<float*>(this->Points->GetVoidPointer(0));
      this->SortedPoints.Initialize(reinterpret_cast<vtkVector2f*>(data),
        this->Points->GetNumberOfPoints());
      std::sort(this->SortedPoints.begin(), this->SortedPoints.end(),
        vtkIndexedVector2f::compVector3fX);
    }
    return this->SortedPoints.GetNearestPoint(point, tol, location);
  }
};

vtkStandardNewMacro(vtkPlotArea);
//----------------------------------------------------------------------------
vtkPlotArea::vtkPlotArea()
  : TableCache(new vtkPlotArea::vtkTableCache())
{
  this->TooltipDefaultLabelFormat = "%l: %x:(%a, %b)";
}

//----------------------------------------------------------------------------
vtkPlotArea::~vtkPlotArea()
{
  delete this->TableCache;
  this->TableCache = NULL;
}

//----------------------------------------------------------------------------
void vtkPlotArea::Update()
{
  if (!this->Visible)
  {
    return;
  }

  vtkTable* table = this->GetInput();
  if (!table)
  {
    vtkDebugMacro("Update event called with no input table set.");
    this->TableCache->Reset();
    return;
  }

  if (this->Data->GetMTime() > this->UpdateTime ||
    table->GetMTime() > this->UpdateTime ||
    this->GetMTime() > this->UpdateTime)
  {
    vtkTableCache& cache = (*this->TableCache);

    cache.Reset();
    cache.ValidPointMask = (this->ValidPointMaskName.empty() == false)?
      vtkArrayDownCast<vtkCharArray>(table->GetColumnByName(this->ValidPointMaskName)) : NULL;
    cache.SetPoints(
      this->UseIndexForXSeries? NULL: this->Data->GetInputArrayToProcess(0, table),
      this->Data->GetInputArrayToProcess(1, table),
      this->Data->GetInputArrayToProcess(2, table));
    this->UpdateTime.Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPlotArea::UpdateCache()
{
  vtkTableCache& cache = (*this->TableCache);
  if (!this->Visible || !cache.IsInputDataValid())
  {
    return;
  }
  cache.UpdateCache(this);
}

//----------------------------------------------------------------------------
void vtkPlotArea::GetBounds(double bounds[4])
{
  vtkTableCache& cache = (*this->TableCache);
  if (!this->Visible || !cache.IsInputDataValid())
  {
    return;
  }
  cache.GetDataBounds(bounds);
}

//----------------------------------------------------------------------------
bool vtkPlotArea::Paint(vtkContext2D *painter)
{
  vtkTableCache& cache = (*this->TableCache);
  if (!this->Visible || !cache.IsInputDataValid() || cache.Points->GetNumberOfPoints() == 0)
  {
    return false;
  }
  painter->ApplyPen(this->Pen);
  painter->ApplyBrush(this->Brush);

  vtkIdType start = 0;
  for (std::vector<vtkIdType>::iterator iter = cache.BadPoints.begin();
    iter != cache.BadPoints.end(); ++iter)
  {
    vtkIdType end = *iter;
    if ((end-start) >= 2)
    {
      painter->DrawQuadStrip(
        reinterpret_cast<float*>(cache.Points->GetVoidPointer(2*2*start)),
        (end-start)*2);
    }
    start = end;
  }
  if (cache.Points->GetNumberOfPoints() - (2*start) > 4)
  {
    painter->DrawQuadStrip(
      reinterpret_cast<float*>(cache.Points->GetVoidPointer(2*2*start)),
      cache.Points->GetNumberOfPoints() - (2*start));
  }
  return true;
}

//-----------------------------------------------------------------------------
bool vtkPlotArea::PaintLegend(vtkContext2D *painter, const vtkRectf& rect,
                              int vtkNotUsed(legendIndex))
{
  painter->ApplyPen(this->Pen);
  painter->ApplyBrush(this->Brush);
  painter->DrawRect(rect[0], rect[1], rect[2], rect[3]);
  return true;
}

//----------------------------------------------------------------------------
vtkIdType vtkPlotArea::GetNearestPoint(
  const vtkVector2f& point, const vtkVector2f& tolerance, vtkVector2f* location)
{
  vtkTableCache& cache = (*this->TableCache);
  if (!this->Visible || !cache.IsInputDataValid() || cache.Points->GetNumberOfPoints() == 0)
  {
    return -1;
  }
  return cache.GetNearestPoint(point, tolerance, location);
}


//-----------------------------------------------------------------------------
vtkStdString vtkPlotArea::GetTooltipLabel(const vtkVector2d &plotPos,
                                      vtkIdType seriesIndex,
                                      vtkIdType segmentIndex)
{
  vtkStdString tooltipLabel;
  vtkStdString format = this->Superclass::GetTooltipLabel(plotPos, seriesIndex, segmentIndex);

  vtkIdType idx = (seriesIndex / 2) * 2;

  vtkTableCache& cache = (*this->TableCache);
  const vtkVector2f* data = reinterpret_cast<vtkVector2f*>(cache.Points->GetVoidPointer(0));

  // Parse TooltipLabelFormat and build tooltipLabel
  bool escapeNext = false;
  for (size_t i = 0; i < format.length(); ++i)
  {
    if (escapeNext)
    {
      switch (format[i])
      {
        case 'a':
          tooltipLabel += this->GetNumber(data[idx].GetY(), this->YAxis);
          break;
        case 'b':
          tooltipLabel += this->GetNumber(data[idx+1].GetY(), this->YAxis);
          break;
        default: // If no match, insert the entire format tag
          tooltipLabel += "%";
          tooltipLabel += format[i];
          break;
      }
      escapeNext = false;
    }
    else
    {
      if (format[i] == '%')
      {
        escapeNext = true;
      }
      else
      {
        tooltipLabel += format[i];
      }
    }
  }
  return tooltipLabel;
}

//----------------------------------------------------------------------------
void vtkPlotArea::SetColor(
  unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  this->Brush->SetColor(r, g, b, a);
  this->Superclass::SetColor(r, g, b, a);
}

//----------------------------------------------------------------------------
void vtkPlotArea::SetColor(double r,  double g, double b)
{
  this->Brush->SetColorF(r, g, b);
  this->Superclass::SetColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkPlotArea::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
