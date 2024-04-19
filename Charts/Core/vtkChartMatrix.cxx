// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkChartMatrix.h"

#include "vtkAxis.h"
#include "vtkBoundingBox.h"
#include "vtkChartXY.h"
#include "vtkContext2D.h"
#include "vtkContextScene.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <unordered_map>
#include <vector>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
class vtkChartMatrix::PIMPL
{
public:
  // Iterator helpers.
  vtkVector2f Increment, Start, Offset;
  vtkVector2i Index;

  // Container for the vtkChart/vtkChartmatrix objects that make up the matrix.
  std::vector<vtkSmartPointer<vtkAbstractContextItem>> ChartElements;
  // Spans of the charts in the matrix, default is 1x1.
  std::vector<vtkVector2i> Spans;

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
  this->Padding = 0.05;
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
  const bool isRoot = (this->Parent == nullptr);
  const bool isEmpty = this->Size.GetX() <= 0 && this->Size.GetY() <= 0;
  bool needRecompute = this->LayoutIsDirty;
  if (isRoot && (this->FillStrategy == StretchType::SCENE))
  {
    needRecompute |= this->GetScene()->GetSceneWidth() != this->Rect.GetWidth();
    needRecompute |= this->GetScene()->GetSceneHeight() != this->Rect.GetHeight();
  }

  if (isEmpty || !needRecompute)
  {
    return Superclass::Paint(painter);
  }

  // Update the chart element positions
  if (isRoot && (this->FillStrategy == StretchType::SCENE))
  {
    this->Rect.SetX(0);
    this->Rect.SetY(0);
    this->Rect.SetWidth(this->GetScene()->GetSceneWidth());
    this->Rect.SetHeight(this->GetScene()->GetSceneHeight());
  }

  vtkVector2i index;
  vtkVector2f offset, increment;
  for (this->InitLayoutTraversal(index, offset, increment); !this->IsDoneWithTraversal();
       this->GoToNextElement(index, offset))
  {
    const vtkRectf& rect = this->ComputeCurrentElementSceneRect(index, offset, increment);
    const std::size_t flatIndex = this->GetFlatIndex(this->Private->Index);
    vtkAbstractContextItem* child = this->Private->ChartElements[flatIndex];
    vtkChart* chart = vtkChart::SafeDownCast(child);
    vtkChartMatrix* cMatrix = vtkChartMatrix::SafeDownCast(child);

    if (chart != nullptr)
    {
      chart->SetSize(rect);
    }
    else if (cMatrix != nullptr)
    {
      cMatrix->SetRect(vtkRecti(rect.Cast<int>().GetData()));
    }
  }

  this->LayoutIsDirty = false;
  return Superclass::Paint(painter);
}

//------------------------------------------------------------------------------
void vtkChartMatrix::SetSize(const vtkVector2i& size)
{
  if (this->Size.GetX() != size.GetX() || this->Size.GetY() != size.GetY())
  {
    this->Size = size;
    if (size.GetX() * size.GetY() < static_cast<int>(this->Private->ChartElements.size()))
    {
      for (int i = static_cast<int>(this->Private->ChartElements.size() - 1);
           i >= size.GetX() * size.GetY(); --i)
      {
        this->RemoveItem(this->Private->ChartElements[i]);
      }
    }
    const std::size_t numCharts = static_cast<std::size_t>(size.GetX()) * size.GetY();
    this->Private->ChartElements.resize(numCharts);
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
void vtkChartMatrix::SetRect(vtkRecti rect)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): setting Rect"
                   " to "
                << rect);
  if (this->Rect != rect)
  {
    this->Rect = rect;
    this->Modified();
    this->LayoutIsDirty = true;
  }
}

//------------------------------------------------------------------------------
vtkRectf vtkChartMatrix::ComputeCurrentElementSceneRect(
  const vtkVector2i& index, const vtkVector2f& offset, const vtkVector2f& increment)
{
  const std::size_t flatIndex = this->GetFlatIndex(index);
  vtkVector2f resize(0., 0.);

  if (this->SpecificResize.find(index) != this->SpecificResize.end())
  {
    resize = this->SpecificResize[index];
  }

  const float x =
    offset[0] + this->Gutter.GetX() * this->Private->GutterCompensation[flatIndex][vtkAxis::LEFT];
  const float y =
    offset[1] + this->Gutter.GetX() * this->Private->GutterCompensation[flatIndex][vtkAxis::BOTTOM];

  vtkVector2i& span = this->Private->Spans[flatIndex];
  float left = x + resize.GetX();
  float bottom = y + resize.GetY();
  float width = increment.GetX() * span.GetX() - resize.GetX() +
    this->Gutter.GetX() *
      (span.GetX() - 1 + this->Private->GutterCompensation[flatIndex][vtkAxis::RIGHT]);
  float height = increment.GetY() * span.GetY() - resize.GetY() +
    this->Gutter.GetY() *
      (span.GetY() - 1 + this->Private->GutterCompensation[flatIndex][vtkAxis::TOP]);

  width = vtkMath::ClampValue(width, 0.f, VTK_FLOAT_MAX);
  height = vtkMath::ClampValue(height, 0.f, VTK_FLOAT_MAX);

  return { left, bottom, width, height };
}

//------------------------------------------------------------------------------
void vtkChartMatrix::InitLayoutTraversal(
  vtkVector2i& index, vtkVector2f& offset, vtkVector2f& increment)
{ // Calculate the increment without the gutters/borders that must be left
  vtkVector2f gutters(0.f, 0.f), borders(0.f, 0.f);

  for (int dim = 0; dim < 2; ++dim)
  {
    gutters[dim] = this->Gutter[dim] * (this->Size[dim] - 1);
    borders[dim] = this->Borders[dim] + this->Borders[dim + 2];
    this->Private->Start[dim] = this->Rect[dim] + this->Borders[dim];
    this->Private->Increment[dim] = this->Rect[dim + 2] - gutters[dim] - borders[dim];
    this->Private->Increment[dim] /= this->Size[dim];
  }

  this->Private->Offset = this->Private->Start;
  this->Private->Index = { 0, 0 };

  increment = this->Private->Increment;
  index = this->Private->Index;
  offset = this->Private->Offset;
}

//------------------------------------------------------------------------------
void vtkChartMatrix::GoToNextElement(vtkVector2i& index, vtkVector2f& offset)
{
  const int& numRows = this->Size.GetY();
  int& i = this->Private->Index[0];
  int& j = this->Private->Index[1];
  const int FIRST = 0;

  // increment j
  ++j;
  // reset j, increment i if necessary
  if (!(j % numRows))
  {
    j = 0;
    ++i;
    // Compute next column's x offset
    switch (i)
    {
      case FIRST:
        break;
      default:
        this->Private->Offset[0] += this->Private->Increment[0] + this->Gutter[0];
        break;
    }
  }

  // Compute next row's y offset
  switch (j)
  {
    case FIRST:
      this->Private->Offset[1] = this->Private->Start[1];
      break;
    default:
      this->Private->Offset[1] += this->Private->Increment[1] + this->Gutter[1];
      break;
  }

  index = this->Private->Index;
  offset = this->Private->Offset;
}

//------------------------------------------------------------------------------
bool vtkChartMatrix::IsDoneWithTraversal()
{
  const int& numCols = this->Size.GetX();
  const int& i = this->Private->Index[0];
  const int& j = this->Private->Index[1];
  return i == numCols && !j;
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
void vtkChartMatrix::SetPadding(const float& padding)
{
  this->Padding = padding;
  this->LayoutIsDirty = true;
  this->Modified();
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
bool vtkChartMatrix::SetChartMatrix(const vtkVector2i& position, vtkChartMatrix* chartMatrix)
{
  if (position.GetX() < this->Size.GetX() && position.GetY() < this->Size.GetY())
  {
    std::size_t index = position.GetY() * this->Size.GetX() + position.GetX();
    if (this->Private->ChartElements[index])
    {
      this->RemoveItem(this->Private->ChartElements[index]);
    }
    this->Private->ChartElements[index] = chartMatrix;
    this->AddItem(chartMatrix);
    return true;
  }
  else
  {
    return false;
  }
}

//------------------------------------------------------------------------------
vtkChartMatrix* vtkChartMatrix::GetChartMatrix(const vtkVector2i& position)
{
  if (position.GetX() < this->Size.GetX() && position.GetY() < this->Size.GetY())
  {
    std::size_t index = position.GetY() * this->Size.GetX() + position.GetX();
    if (this->Private->ChartElements[index] == nullptr)
    {
      vtkNew<vtkChartMatrix> chartMatrix;
      this->Private->ChartElements[index] = chartMatrix;
      this->AddItem(chartMatrix);
    }
    return vtkChartMatrix::SafeDownCast(this->Private->ChartElements[index]);
  }
  else
  {
    return nullptr;
  }
}

//------------------------------------------------------------------------------
bool vtkChartMatrix::SetChart(const vtkVector2i& position, vtkChart* chart)
{
  if (position.GetX() < this->Size.GetX() && position.GetY() < this->Size.GetY())
  {
    std::size_t index = position.GetY() * this->Size.GetX() + position.GetX();
    if (this->Private->ChartElements[index])
    {
      this->RemoveItem(this->Private->ChartElements[index]);
    }
    this->Private->ChartElements[index] = chart;
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
    std::size_t index = position.GetY() * this->Size.GetX() + position.GetX();
    if (this->Private->ChartElements[index] == nullptr)
    {
      vtkNew<vtkChartXY> chart;
      this->Private->ChartElements[index] = chart;
      this->AddItem(chart);
      chart->SetLayoutStrategy(vtkChart::AXES_TO_RECT);
    }
    return vtkChart::SafeDownCast(this->Private->ChartElements[index]);
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
  const bool isEmpty = this->Size.GetX() <= 0 && this->Size.GetY() <= 0;
  if (isEmpty)
  {
    return { -1, -1 };
  }

  vtkVector2i index;
  vtkVector2f offset, increment;

  for (this->InitLayoutTraversal(index, offset, increment); !this->IsDoneWithTraversal();
       this->GoToNextElement(index, offset))
  {
    const vtkRectf& rect = this->ComputeCurrentElementSceneRect(index, offset, increment);
    vtkBoundingBox bbox(rect.GetLeft(), rect.GetRight(), rect.GetBottom(), rect.GetTop(),
      VTK_DOUBLE_MIN, VTK_DOUBLE_MAX);
    if (bbox.ContainsPoint(position.GetX(), position.GetY(), 0.0))
    {
      return this->Private->Index;
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
  return this->Private->ChartElements.size();
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
      vtkChart* chart = vtkChart::SafeDownCast(this->Private->ChartElements[cid]);
      if (chart == nullptr)
      {
        continue;
      }

      chartIds.push_back(cid);

      const bool leftDecorationsVisible = i == left;
      const bool rightDecorationsVisible = i == right;
      const bool topDecorationsVisible = j == top;
      const bool bottomDecorationsVisible = j == bottom;

      chart->GetAxis(vtkAxis::LEFT)->SetLabelsVisible(leftDecorationsVisible);
      chart->GetAxis(vtkAxis::LEFT)->SetTitleVisible(leftDecorationsVisible);
      chart->GetAxis(vtkAxis::RIGHT)->SetLabelsVisible(rightDecorationsVisible);
      chart->GetAxis(vtkAxis::RIGHT)->SetTitleVisible(rightDecorationsVisible);
      chart->GetAxis(vtkAxis::TOP)->SetLabelsVisible(topDecorationsVisible);
      chart->GetAxis(vtkAxis::TOP)->SetTitleVisible(topDecorationsVisible);
      chart->GetAxis(vtkAxis::BOTTOM)->SetLabelsVisible(bottomDecorationsVisible);
      chart->GetAxis(vtkAxis::BOTTOM)->SetTitleVisible(bottomDecorationsVisible);

      if (i > left)
      {
        this->Private->GutterCompensation[cid][vtkAxis::LEFT] = -0.5;
      }
      if (i < right)
      {
        this->Private->GutterCompensation[cid][vtkAxis::RIGHT] = 1.f;
      }
      if (j > bottom)
      {
        this->Private->GutterCompensation[cid][vtkAxis::BOTTOM] = -0.5f;
      }
      if (j < top)
      {
        this->Private->GutterCompensation[cid][vtkAxis::TOP] = 1.f;
      }

      if (i == left)
      {
        this->Private->GutterCompensation[cid][vtkAxis::LEFT] = 0.f;
        this->Private->GutterCompensation[cid][vtkAxis::RIGHT] = 0.5f;
      }
      if (i == right)
      {
        this->Private->GutterCompensation[cid][vtkAxis::RIGHT] = 0.5f;
      }
      if (j == bottom)
      {
        this->Private->GutterCompensation[cid][vtkAxis::BOTTOM] = 0.f;
        this->Private->GutterCompensation[cid][vtkAxis::TOP] = 0.5f;
      }
      if (j == top)
      {
        this->Private->GutterCompensation[cid][vtkAxis::TOP] = 0.5f;
      }
    }
  }

  if (chartIds.empty())
  {
    return;
  }
  chartIds.push_back(chartIds.front()); // cycle
  for (auto chartIt = chartIds.cbegin(); chartIt != std::prev(chartIds.end());)
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

  vtkChart* chart = vtkChart::SafeDownCast(this->Private->ChartElements[flatIndex1]);
  if (chart == nullptr)
  {
    return;
  }

  if (axis % 2) // bottom, top
  {
    this->Private->XAxisRangeObserverTags[flatIndex1].insert({ flatIndex2,
      chart->AddObserver(vtkChart::UpdateRange, this, &vtkChartMatrix::SynchronizeAxisRanges) });
  }
  else // left, right
  {
    this->Private->YAxisRangeObserverTags[flatIndex1].insert({ flatIndex2,
      chart->AddObserver(vtkChart::UpdateRange, this, &vtkChartMatrix::SynchronizeAxisRanges) });
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
  for (std::size_t i = 0; i < this->Private->ChartElements.size(); ++i)
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

  vtkChart* chart = vtkChart::SafeDownCast(this->Private->ChartElements[flatIndex1]);
  if (chart == nullptr)
  {
    return;
  }

  if (axis % 2) // bottom, top
  {
    if (this->Private->XAxisRangeObserverTags[flatIndex1].find(flatIndex2) !=
      this->Private->XAxisRangeObserverTags[flatIndex1].end())
    {
      chart->RemoveObserver(this->Private->XAxisRangeObserverTags[flatIndex1][flatIndex2]);
      this->Private->XAxisRangeObserverTags[flatIndex1].erase(flatIndex2);
    }
  }
  else // left, right, parallel
  {
    if (this->Private->YAxisRangeObserverTags[flatIndex1].find(flatIndex2) !=
      this->Private->YAxisRangeObserverTags[flatIndex1].end())
    {
      chart->RemoveObserver(this->Private->YAxisRangeObserverTags[flatIndex1][flatIndex2]);
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
  for (std::size_t i = 0; i < this->Private->ChartElements.size(); ++i)
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
  for (std::size_t cid = 0; cid < this->Private->ChartElements.size(); ++cid)
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
  for (std::size_t cid = 0; cid < this->Private->ChartElements.size(); ++cid)
  {
    for (int axId = 0; axId < 4; ++axId)
    {
      this->Private->GutterCompensation[cid][axId] = 0.0f;
    }

    vtkChart* chart = vtkChart::SafeDownCast(this->Private->ChartElements[cid]);
    if (chart == nullptr)
    {
      continue;
    }

    chart->GetAxis(vtkAxis::BOTTOM)->SetLabelsVisible(true);
    chart->GetAxis(vtkAxis::BOTTOM)->SetTitleVisible(true);
    chart->GetAxis(vtkAxis::LEFT)->SetLabelsVisible(true);
    chart->GetAxis(vtkAxis::LEFT)->SetTitleVisible(true);
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
  auto source =
    vtkAbstractContextItem::SafeDownCast(caller); // the source chart of UpdateRange event.
  if (!source)
  {
    return;
  }
  const auto sourceAt =
    std::find(this->Private->ChartElements.begin(), this->Private->ChartElements.end(), source);
  auto sourceIdx = std::distance(this->Private->ChartElements.begin(), sourceAt);
  if (!this->Private->OngoingRangeUpdates[sourceIdx])
  {
    // Block all events into sourceIdx chart
    this->Private->BlockUpdateRangeEvents(sourceIdx);
    // Synchronize.
    double* fullAxisRange = reinterpret_cast<double*>(calldata);
    for (const auto& observerInfo : this->Private->XAxisRangeObserverTags[sourceIdx])
    {
      vtkChart* observerChart =
        vtkChart::SafeDownCast(this->Private->ChartElements[observerInfo.first]);
      if (observerChart)
      {
        observerChart->GetAxis(vtkAxis::BOTTOM)->SetRange(&fullAxisRange[vtkAxis::BOTTOM * 2]);
        observerChart->GetAxis(vtkAxis::TOP)->SetRange(&fullAxisRange[vtkAxis::TOP * 2]);
      }
    }
    for (const auto& observerInfo : this->Private->YAxisRangeObserverTags[sourceIdx])
    {
      vtkChart* observerChart =
        vtkChart::SafeDownCast(this->Private->ChartElements[observerInfo.first]);
      if (observerChart != nullptr)
      {
        observerChart->GetAxis(vtkAxis::LEFT)->SetRange(&fullAxisRange[vtkAxis::LEFT * 2]);
        observerChart->GetAxis(vtkAxis::RIGHT)->SetRange(&fullAxisRange[vtkAxis::RIGHT * 2]);
      }
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
VTK_ABI_NAMESPACE_END
