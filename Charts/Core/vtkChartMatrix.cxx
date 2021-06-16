/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChartMatrix.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkChartMatrix.h"

#include "vtkAxis.h"
#include "vtkChartXY.h"
#include "vtkContext2D.h"
#include "vtkContextScene.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <unordered_map>
#include <vector>

//------------------------------------------------------------------------------
class vtkChartMatrix::PIMPL
{
public:
  PIMPL()
    : Geometry(0, 0)
  {
  }
  ~PIMPL() = default;

  // Container for the vtkChart objects that make up the matrix.
  std::vector<vtkSmartPointer<vtkChart>> Charts;
  // Spans of the charts in the matrix, default is 1x1.
  std::vector<vtkVector2i> Spans;
  vtkVector2i Geometry;

  // Every linked chart observes every other chart for UpdateRange event.
  std::vector<std::unordered_map<std::size_t, int>> XAxisRangeObserverTags, YAxisRangeObserverTags;
  // To prevent infinite callbacks when each 'linked' chart emits UpdateRange
  std::vector<bool> OngoingRangeUpdates;
  std::vector<std::array<float, 4>> GutterCompensation;

  void BlockUpdateRangeEvents(std::size_t idx) { this->OngoingRangeUpdates[idx] = true; }

  void UnblockUpdateRangeEvents(std::size_t idx) { this->OngoingRangeUpdates[idx] = false; }
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkChartMatrix);

//------------------------------------------------------------------------------
vtkChartMatrix::vtkChartMatrix()
  : Size(0, 0)
  , Gutter(15.0, 15.0)
{
  this->Private = new PIMPL;
  this->Borders[vtkAxis::LEFT] = 50;
  this->Borders[vtkAxis::BOTTOM] = 40;
  this->Borders[vtkAxis::RIGHT] = 50;
  this->Borders[vtkAxis::TOP] = 40;
  this->LayoutIsDirty = true;
}

//------------------------------------------------------------------------------
vtkChartMatrix::~vtkChartMatrix()
{
  delete this->Private;
}

//------------------------------------------------------------------------------
void vtkChartMatrix::Update() {}

//------------------------------------------------------------------------------
bool vtkChartMatrix::Paint(vtkContext2D* painter)
{
  if (this->LayoutIsDirty || this->GetScene()->GetSceneWidth() != this->Private->Geometry.GetX() ||
    this->GetScene()->GetSceneHeight() != this->Private->Geometry.GetY())
  {
    // Update the chart element positions
    this->Private->Geometry.Set(
      this->GetScene()->GetSceneWidth(), this->GetScene()->GetSceneHeight());
    if (this->Size.GetX() > 0 && this->Size.GetY() > 0)
    {
      // Calculate the increments without the gutters/borders that must be left
      vtkVector2f gutters(0.f, 0.f), borders(0.f, 0.f), increments(0.f, 0.f);
      for (int dim = 0; dim < 2; ++dim)
      {
        gutters[dim] = this->Gutter[dim] * (this->Size[dim] - 1);
        borders[dim] = this->Borders[dim] + this->Borders[dim + 2];
        increments[dim] = this->Private->Geometry[dim] - gutters[dim] - borders[dim];
        increments[dim] /= this->Size[dim];
      }

      float x = this->Borders[vtkAxis::LEFT];
      float y = this->Borders[vtkAxis::BOTTOM];
      for (int i = 0; i < this->Size.GetX(); ++i)
      {
        if (i > 0)
        {
          x += increments.GetX() + this->Gutter.GetX();
        }
        for (int j = 0; j < this->Size.GetY(); ++j)
        {
          if (j > 0)
          {
            y += increments.GetY() + this->Gutter.GetY();
          }
          else
          {
            y = this->Borders[vtkAxis::BOTTOM];
          }
          vtkVector2f resize(0., 0.);
          vtkVector2i key(i, j);
          if (this->SpecificResize.find(key) != this->SpecificResize.end())
          {
            resize = this->SpecificResize[key];
          }
          size_t index = this->GetFlatIndex({ i, j });
          x += this->Gutter.GetX() * this->Private->GutterCompensation[index][vtkAxis::LEFT];
          y += this->Gutter.GetX() * this->Private->GutterCompensation[index][vtkAxis::BOTTOM];
          if (this->Private->Charts[index])
          {
            vtkChart* chart = this->Private->Charts[index];
            vtkVector2i& span = this->Private->Spans[index];
            vtkRectf chartRect(x + resize.GetX(), y + resize.GetY(),
              increments.GetX() * span.GetX() - resize.GetX() +
                (span.GetX() - 1 + this->Private->GutterCompensation[index][vtkAxis::RIGHT]) *
                  this->Gutter.GetX(),
              increments.GetY() * span.GetY() - resize.GetY() +
                (span.GetY() - 1 + this->Private->GutterCompensation[index][vtkAxis::TOP]) *
                  this->Gutter.GetY());
            // ensure that the size is valid.If not, make the rect an empty rect.
            if (chartRect.GetWidth() < 0)
            {
              chartRect.SetWidth(0);
            }
            if (chartRect.GetHeight() < 0)
            {
              chartRect.SetHeight(0);
            }
            chart->SetSize(chartRect);
          }
          x -= this->Gutter.GetX() * this->Private->GutterCompensation[index][vtkAxis::LEFT];
          y -= this->Gutter.GetX() * this->Private->GutterCompensation[index][vtkAxis::BOTTOM];
        }
      }
    }
    this->LayoutIsDirty = false;
  }
  return Superclass::Paint(painter);
}

//------------------------------------------------------------------------------
void vtkChartMatrix::SetSize(const vtkVector2i& size)
{
  if (this->Size.GetX() != size.GetX() || this->Size.GetY() != size.GetY())
  {
    this->Size = size;
    if (size.GetX() * size.GetY() < static_cast<int>(this->Private->Charts.size()))
    {
      for (int i = static_cast<int>(this->Private->Charts.size() - 1);
           i >= size.GetX() * size.GetY(); --i)
      {
        this->RemoveItem(this->Private->Charts[i]);
      }
    }
    const std::size_t numCharts = static_cast<std::size_t>(size.GetX()) * size.GetY();
    this->Private->Charts.resize(numCharts);
    this->Private->Spans.resize(numCharts, vtkVector2i(1, 1));
    this->Private->XAxisRangeObserverTags.resize(numCharts);
    this->Private->YAxisRangeObserverTags.resize(numCharts);
    this->Private->OngoingRangeUpdates.resize(numCharts, false);
    this->Private->GutterCompensation.resize(
      numCharts, std::array<float, 4>({ 0.0f, 0.0f, 0.0f, 0.0f }));
    this->LayoutIsDirty = true;
  }
}

//------------------------------------------------------------------------------
void vtkChartMatrix::SetBorders(int left, int bottom, int right, int top)
{
  this->Borders[vtkAxis::LEFT] = left;
  this->Borders[vtkAxis::BOTTOM] = bottom;
  this->Borders[vtkAxis::RIGHT] = right;
  this->Borders[vtkAxis::TOP] = top;
  this->LayoutIsDirty = true;
}

//------------------------------------------------------------------------------
void vtkChartMatrix::SetBorderLeft(int value)
{
  this->Borders[vtkAxis::LEFT] = value;
  this->LayoutIsDirty = true;
}

//------------------------------------------------------------------------------
void vtkChartMatrix::SetBorderBottom(int value)
{
  this->Borders[vtkAxis::BOTTOM] = value;
  this->LayoutIsDirty = true;
}

//------------------------------------------------------------------------------
void vtkChartMatrix::SetBorderRight(int value)
{
  this->Borders[vtkAxis::RIGHT] = value;
  this->LayoutIsDirty = true;
}

//------------------------------------------------------------------------------
void vtkChartMatrix::SetBorderTop(int value)
{
  this->Borders[vtkAxis::TOP] = value;
  this->LayoutIsDirty = true;
}

//------------------------------------------------------------------------------
void vtkChartMatrix::SetGutter(const vtkVector2f& gutter)
{
  this->Gutter = gutter;
  this->LayoutIsDirty = true;
}

//------------------------------------------------------------------------------
void vtkChartMatrix::SetGutterX(float value)
{
  this->Gutter.SetX(value);
  this->LayoutIsDirty = true;
}

//------------------------------------------------------------------------------
void vtkChartMatrix::SetGutterY(float value)
{
  this->Gutter.SetY(value);
  this->LayoutIsDirty = true;
}

//------------------------------------------------------------------------------
void vtkChartMatrix::SetSpecificResize(const vtkVector2i& index, const vtkVector2f& resize)
{
  if (this->SpecificResize.find(index) == this->SpecificResize.end() ||
    this->SpecificResize[index] != resize)
  {
    this->SpecificResize[index] = resize;
    this->LayoutIsDirty = true;
  }
}

//------------------------------------------------------------------------------
void vtkChartMatrix::ClearSpecificResizes()
{
  if (!this->SpecificResize.empty())
  {
    this->SpecificResize.clear();
    this->LayoutIsDirty = true;
  }
}

//------------------------------------------------------------------------------
void vtkChartMatrix::Allocate()
{
  // Force allocation of all objects as vtkChartXY.
}

//------------------------------------------------------------------------------
bool vtkChartMatrix::SetChart(const vtkVector2i& position, vtkChart* chart)
{
  if (position.GetX() < this->Size.GetX() && position.GetY() < this->Size.GetY())
  {
    size_t index = position.GetY() * this->Size.GetX() + position.GetX();
    if (this->Private->Charts[index])
    {
      this->RemoveItem(this->Private->Charts[index]);
    }
    this->Private->Charts[index] = chart;
    this->AddItem(chart);
    chart->SetLayoutStrategy(vtkChart::AXES_TO_RECT);
    return true;
  }
  else
  {
    return false;
  }
}

//------------------------------------------------------------------------------
vtkChart* vtkChartMatrix::GetChart(const vtkVector2i& position)
{
  if (position.GetX() < this->Size.GetX() && position.GetY() < this->Size.GetY())
  {
    size_t index = position.GetY() * this->Size.GetX() + position.GetX();
    if (this->Private->Charts[index] == nullptr)
    {
      vtkNew<vtkChartXY> chart;
      this->Private->Charts[index] = chart;
      this->AddItem(chart);
      chart->SetLayoutStrategy(vtkChart::AXES_TO_RECT);
    }
    return this->Private->Charts[index];
  }
  else
  {
    return nullptr;
  }
}

//------------------------------------------------------------------------------
bool vtkChartMatrix::SetChartSpan(const vtkVector2i& position, const vtkVector2i& span)
{
  if (this->Size.GetX() - position.GetX() - span.GetX() < 0 ||
    this->Size.GetY() - position.GetY() - span.GetY() < 0)
  {
    return false;
  }
  else
  {
    this->Private->Spans[position.GetY() * this->Size.GetX() + position.GetX()] = span;
    this->LayoutIsDirty = true;
    return true;
  }
}

//------------------------------------------------------------------------------
vtkVector2i vtkChartMatrix::GetChartSpan(const vtkVector2i& position)
{
  size_t index = position.GetY() * this->Size.GetX() + position.GetX();
  if (position.GetX() < this->Size.GetX() && position.GetY() < this->Size.GetY())
  {
    return this->Private->Spans[index];
  }
  else
  {
    return vtkVector2i(0, 0);
  }
}

//------------------------------------------------------------------------------
vtkVector2i vtkChartMatrix::GetChartIndex(const vtkVector2f& position)
{
  if (this->Size.GetX() > 0 && this->Size.GetY() > 0)
  {
    // Calculate the increments without the gutters/borders that must be left
    vtkVector2f gutters(0.f, 0.f), borders(0.f, 0.f), increments(0.f, 0.f);
    for (int dim = 0; dim < 2; ++dim)
    {
      gutters[dim] = this->Gutter[dim] * (this->Size[dim] - 1);
      borders[dim] = this->Borders[dim] + this->Borders[dim + 2];
      increments[dim] = this->Private->Geometry[dim] - gutters[dim] - borders[dim];
      increments[dim] /= this->Size[dim];
    }

    float x = this->Borders[vtkAxis::LEFT];
    float y = this->Borders[vtkAxis::BOTTOM];
    for (int i = 0; i < this->Size.GetX(); ++i)
    {
      if (i > 0)
      {
        x += increments.GetX() + this->Gutter.GetX();
      }
      for (int j = 0; j < this->Size.GetY(); ++j)
      {
        if (j > 0)
        {
          y += increments.GetY() + this->Gutter.GetY();
        }
        else
        {
          y = this->Borders[vtkAxis::BOTTOM];
        }
        vtkVector2f resize(0., 0.);
        vtkVector2i key(i, j);
        if (this->SpecificResize.find(key) != this->SpecificResize.end())
        {
          resize = this->SpecificResize[key];
        }
        std::size_t index = this->GetFlatIndex({ i, j });
        x += this->Gutter.GetX() * this->Private->GutterCompensation[index][vtkAxis::LEFT];
        y += this->Gutter.GetX() * this->Private->GutterCompensation[index][vtkAxis::BOTTOM];
        if (this->Private->Charts[index])
        {
          vtkVector2i& span = this->Private->Spans[index];
          // Check if the supplied location is within this charts area.
          float x2 = x + resize.GetX();
          float y2 = y + resize.GetY();
          if (position.GetX() > x2 &&
            position.GetX() <
              (x2 + increments.GetX() * span.GetX() - resize.GetY() +
                (span.GetX() - 1 + this->Private->GutterCompensation[index][vtkAxis::RIGHT]) *
                  this->Gutter.GetX()) &&
            position.GetY() > y2 &&
            position.GetY() <
              (y2 + increments.GetY() * span.GetY() - resize.GetY() +
                (span.GetY() - 1 + this->Private->GutterCompensation[index][vtkAxis::TOP]) *
                  this->Gutter.GetY()))
            return vtkVector2i(i, j);
        }
        x -= this->Gutter.GetX() * this->Private->GutterCompensation[index][vtkAxis::LEFT];
        y -= this->Gutter.GetX() * this->Private->GutterCompensation[index][vtkAxis::BOTTOM];
      }
    }
  }
  return vtkVector2i(-1, -1);
}

//------------------------------------------------------------------------------
std::size_t vtkChartMatrix::GetFlatIndex(const vtkVector2i& index)
{
  return static_cast<std::size_t>(index.GetY()) * this->Size.GetX() + index.GetX();
}

//------------------------------------------------------------------------------
std::size_t vtkChartMatrix::GetNumberOfCharts()
{
  return this->Private->Charts.size();
}

//------------------------------------------------------------------------------
void vtkChartMatrix::LabelOuter(const vtkVector2i& leftBottomIdx, const vtkVector2i& rightTopIdx)
{
  // verify valid positions
  if (leftBottomIdx.GetX() > rightTopIdx.GetX())
  {
    return;
  }
  if (leftBottomIdx.GetY() > rightTopIdx.GetY())
  {
    return;
  }
  // by default share x, y
  bool shareX = true, shareY = true;
  if (leftBottomIdx.GetX() == rightTopIdx.GetX())
  {
    shareY = false; // same column
  }
  if (leftBottomIdx.GetY() == rightTopIdx.GetY())
  {
    shareX = false; // same row
  }
  std::vector<std::size_t> chartIds;
  const int& left = leftBottomIdx.GetX();
  const int& right = rightTopIdx.GetX();
  const int& bottom = leftBottomIdx.GetY();
  const int& top = rightTopIdx.GetY();
  for (int i = left; i <= right; ++i)
  {
    for (int j = bottom; j <= top; ++j)
    {
      const auto cid = this->GetFlatIndex({ i, j });
      chartIds.push_back(cid);
      if (i > left && shareY)
      {
        this->Private->GutterCompensation[cid][vtkAxis::LEFT] = -0.5;
        this->Private->Charts[cid]->GetAxis(vtkAxis::LEFT)->SetLabelsVisible(false);
        this->Private->Charts[cid]->GetAxis(vtkAxis::LEFT)->SetTitleVisible(false);
      }
      if (i < right && shareY)
      {
        this->Private->GutterCompensation[cid][vtkAxis::RIGHT] = 1.f;
        this->Private->Charts[cid]->GetAxis(vtkAxis::RIGHT)->SetLabelsVisible(false);
        this->Private->Charts[cid]->GetAxis(vtkAxis::RIGHT)->SetTitleVisible(false);
      }
      if (j > bottom && shareX)
      {
        this->Private->GutterCompensation[cid][vtkAxis::BOTTOM] = -0.5f;
        this->Private->Charts[cid]->GetAxis(vtkAxis::BOTTOM)->SetLabelsVisible(false);
        this->Private->Charts[cid]->GetAxis(vtkAxis::BOTTOM)->SetTitleVisible(false);
      }
      if (j < top && shareX)
      {
        this->Private->GutterCompensation[cid][vtkAxis::TOP] = 1.f;
        this->Private->Charts[cid]->GetAxis(vtkAxis::TOP)->SetLabelsVisible(false);
        this->Private->Charts[cid]->GetAxis(vtkAxis::TOP)->SetTitleVisible(false);
      }
      if (i == left && shareY)
      {
        this->Private->GutterCompensation[cid][vtkAxis::LEFT] = 0.f;
        this->Private->GutterCompensation[cid][vtkAxis::RIGHT] = 0.5f;
        this->Private->Charts[cid]->GetAxis(vtkAxis::LEFT)->SetLabelsVisible(true);
        this->Private->Charts[cid]->GetAxis(vtkAxis::LEFT)->SetTitleVisible(true);
      }
      if (i == right && shareY)
      {
        this->Private->GutterCompensation[cid][vtkAxis::RIGHT] = 0.5f;
        this->Private->Charts[cid]->GetAxis(vtkAxis::RIGHT)->SetLabelsVisible(true);
        this->Private->Charts[cid]->GetAxis(vtkAxis::RIGHT)->SetTitleVisible(true);
      }
      if (j == bottom && shareX)
      {
        this->Private->GutterCompensation[cid][vtkAxis::BOTTOM] = 0.f;
        this->Private->GutterCompensation[cid][vtkAxis::TOP] = 0.5f;
        this->Private->Charts[cid]->GetAxis(vtkAxis::BOTTOM)->SetLabelsVisible(true);
        this->Private->Charts[cid]->GetAxis(vtkAxis::BOTTOM)->SetTitleVisible(true);
      }
      if (j == top && shareX)
      {
        this->Private->GutterCompensation[cid][vtkAxis::TOP] = 0.5f;
        this->Private->Charts[cid]->GetAxis(vtkAxis::TOP)->SetLabelsVisible(true);
        this->Private->Charts[cid]->GetAxis(vtkAxis::TOP)->SetTitleVisible(true);
      }
    }
  }
  auto chartIt = chartIds.cbegin();
  for (; chartIt != std::prev(chartIds.end());)
  {
    const auto& c1id = *chartIt++;
    const auto& c2id = *chartIt;
    if (shareY)
    {
      this->Link(c1id, c2id, vtkAxis::LEFT);
      this->Link(c2id, c1id, vtkAxis::LEFT);
    }
    if (shareX)
    {
      this->Link(c1id, c2id, vtkAxis::BOTTOM);
      this->Link(c2id, c1id, vtkAxis::BOTTOM);
    }
  }
  if (shareY)
  {
    this->Link(chartIds.front(), chartIds.back(), vtkAxis::LEFT);
    this->Link(chartIds.back(), chartIds.front(), vtkAxis::LEFT);
  }
  if (shareX)
  {
    this->Link(chartIds.front(), chartIds.back(), vtkAxis::BOTTOM);
    this->Link(chartIds.back(), chartIds.front(), vtkAxis::BOTTOM);
  }
  this->LayoutIsDirty = true;
}

//------------------------------------------------------------------------------
void vtkChartMatrix::Link(const vtkVector2i& index1, const vtkVector2i& index2, int axis /*=1*/)
{
  const auto& flatIndex1 = this->GetFlatIndex(index1);
  const auto& flatIndex2 = this->GetFlatIndex(index2);
  this->Link(flatIndex1, flatIndex2, axis);
}

//------------------------------------------------------------------------------
void vtkChartMatrix::Link(const size_t& flatIndex1, const size_t& flatIndex2, int axis /*=1*/)
{
  if (flatIndex1 == flatIndex2)
  {
    return;
  }
  const auto& chart1 = this->Private->Charts[flatIndex1];
  if (axis % 2) // bottom, top
  {
    this->Private->XAxisRangeObserverTags[flatIndex1].insert({ flatIndex2,
      chart1->AddObserver(vtkChart::UpdateRange, this, &vtkChartMatrix::SynchronizeAxisRanges) });
  }
  else // left, right
  {
    this->Private->YAxisRangeObserverTags[flatIndex1].insert({ flatIndex2,
      chart1->AddObserver(vtkChart::UpdateRange, this, &vtkChartMatrix::SynchronizeAxisRanges) });
  }
  this->LayoutIsDirty = true;
}

//------------------------------------------------------------------------------
void vtkChartMatrix::LinkAll(const vtkVector2i& index, int axis /*=1*/)
{
  this->LinkAll(this->GetFlatIndex(index), axis);
}

//------------------------------------------------------------------------------
void vtkChartMatrix::LinkAll(const size_t& flatIndex, int axis /*=1*/)
{
  for (std::size_t i = 0; i < this->Private->Charts.size(); ++i)
  {
    if (i != flatIndex)
    {
      this->Link(i, flatIndex, axis);
      this->Link(flatIndex, i, axis);
    }
  }
}

//------------------------------------------------------------------------------
void vtkChartMatrix::Unlink(const vtkVector2i& index1, const vtkVector2i& index2, int axis /*=1*/)
{
  this->Unlink(this->GetFlatIndex(index1), this->GetFlatIndex(index2), axis);
}

//------------------------------------------------------------------------------
void vtkChartMatrix::Unlink(const size_t& flatIndex1, const size_t& flatIndex2, int axis /*=1*/)
{
  if (flatIndex1 == flatIndex2)
  {
    return;
  }
  const auto& chart1 = this->Private->Charts[flatIndex1];
  if (axis % 2) // bottom, top
  {
    if (this->Private->XAxisRangeObserverTags[flatIndex1].find(flatIndex2) !=
      this->Private->XAxisRangeObserverTags[flatIndex1].end())
    {
      chart1->RemoveObserver(this->Private->XAxisRangeObserverTags[flatIndex1][flatIndex2]);
      this->Private->XAxisRangeObserverTags[flatIndex1].erase(flatIndex2);
    }
  }
  else // left, right, parallel
  {
    if (this->Private->YAxisRangeObserverTags[flatIndex1].find(flatIndex2) !=
      this->Private->YAxisRangeObserverTags[flatIndex1].end())
    {
      chart1->RemoveObserver(this->Private->YAxisRangeObserverTags[flatIndex1][flatIndex2]);
      this->Private->YAxisRangeObserverTags[flatIndex1].erase(flatIndex2);
    }
  }
  this->LayoutIsDirty = true;
}

//------------------------------------------------------------------------------
void vtkChartMatrix::UnlinkAll(const vtkVector2i& index, int axis /*=1*/)
{
  this->UnlinkAll(this->GetFlatIndex(index), axis);
}

//------------------------------------------------------------------------------
void vtkChartMatrix::UnlinkAll(const size_t& flatIndex, int axis /*=1*/)
{
  for (std::size_t i = 0; i < this->Private->Charts.size(); ++i)
  {
    if (i != flatIndex)
    {
      this->Unlink(i, flatIndex, axis);
      this->Unlink(flatIndex, i, axis);
    }
  }
}

//------------------------------------------------------------------------------
void vtkChartMatrix::ResetLinks(int axis /*=1*/)
{
  for (std::size_t cid = 0; cid < this->Private->Charts.size(); ++cid)
  {
    this->UnlinkAll(cid, axis);
  }
  if (axis % 2) // bottom, top
  {
    for (auto& xRangeObserver : this->Private->XAxisRangeObserverTags)
    {
      xRangeObserver.clear();
    }
  }
  else // left, right, parallel
  {
    for (auto& yRangeObserver : this->Private->YAxisRangeObserverTags)
    {
      yRangeObserver.clear();
    }
  }
  this->Private->OngoingRangeUpdates.resize(this->GetNumberOfCharts(), false);
}

//------------------------------------------------------------------------------
void vtkChartMatrix::ResetLinkedLayout()
{
  for (std::size_t cid = 0; cid < this->Private->Charts.size(); ++cid)
  {
    for (int axId = 0; axId < 4; ++axId)
    {
      this->Private->GutterCompensation[cid][axId] = 0.0f;
    }
    this->Private->Charts[cid]->GetAxis(vtkAxis::BOTTOM)->SetLabelsVisible(true);
    this->Private->Charts[cid]->GetAxis(vtkAxis::BOTTOM)->SetTitleVisible(true);
    this->Private->Charts[cid]->GetAxis(vtkAxis::LEFT)->SetLabelsVisible(true);
    this->Private->Charts[cid]->GetAxis(vtkAxis::LEFT)->SetTitleVisible(true);
  }
  for (int axId = 0; axId < 4; ++axId)
  {
    this->ResetLinks(axId);
  }
}

//------------------------------------------------------------------------------
void vtkChartMatrix::SynchronizeAxisRanges(vtkObject* caller, unsigned long eventId, void* calldata)
{
  if (eventId != vtkChart::UpdateRange)
  {
    return;
  }
  auto source = vtkChart::SafeDownCast(caller); // the source chart of UpdateRange event.
  if (!source)
  {
    return;
  }
  const auto sourceAt =
    std::find(this->Private->Charts.begin(), this->Private->Charts.end(), source);
  auto sourceIdx = std::distance(this->Private->Charts.begin(), sourceAt);
  if (!this->Private->OngoingRangeUpdates[sourceIdx])
  {
    // Block all events into rootIdx chart
    this->Private->BlockUpdateRangeEvents(sourceIdx);
    // Synchronize.
    double* fullAxisRange = reinterpret_cast<double*>(calldata);
    for (const auto& observerInfo : this->Private->XAxisRangeObserverTags[sourceIdx])
    {
      const auto& observerChart = this->Private->Charts[observerInfo.first];
      observerChart->GetAxis(vtkAxis::BOTTOM)->SetRange(&fullAxisRange[vtkAxis::BOTTOM * 2]);
      observerChart->GetAxis(vtkAxis::TOP)->SetRange(&fullAxisRange[vtkAxis::TOP * 2]);
    }
    for (const auto& observerInfo : this->Private->YAxisRangeObserverTags[sourceIdx])
    {
      const auto& observerChart = this->Private->Charts[observerInfo.first];
      observerChart->GetAxis(vtkAxis::LEFT)->SetRange(&fullAxisRange[vtkAxis::LEFT * 2]);
      observerChart->GetAxis(vtkAxis::RIGHT)->SetRange(&fullAxisRange[vtkAxis::RIGHT * 2]);
    }
    // Unblock.
    this->Private->UnblockUpdateRangeEvents(sourceIdx);
  }
}

//------------------------------------------------------------------------------
void vtkChartMatrix::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
