// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkEqualizerContextItem.h"

#include "vtkBrush.h"
#include "vtkCommand.h"
#include "vtkContext2D.h"
#include "vtkContextKeyEvent.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkContextTransform.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkVector.h"

#include <limits>
#include <set>
#include <sstream>
#include <vector>

namespace equalizer
{
VTK_ABI_NAMESPACE_BEGIN
struct EqualizerPoint
{
  static const int radius{ 4 };
  static const int radiusInteractive{ 6 };
  int freq{ -1 };
  float coef{ 1.0 };

  //------------------------------------------------------------------------------
  EqualizerPoint(int f, double c)
  {
    freq = f;
    coef = c;
  }

  //------------------------------------------------------------------------------
  EqualizerPoint(const vtkVector2f& vec)
    : freq(vec.GetX())
    , coef(vec.GetY())
  {
  }

  //------------------------------------------------------------------------------
  operator vtkVector2f() const { return vtkVector2f(freq, coef); }

  //------------------------------------------------------------------------------
  EqualizerPoint& operator=(const vtkVector2f& pos)
  {
    this->freq = pos.GetX();
    this->coef = pos.GetY();

    return *this;
  }

  //------------------------------------------------------------------------------
  bool operator<(const EqualizerPoint& point) const { return this->freq < point.freq; }
};

//------------------------------------------------------------------------------
bool isNear(vtkVector2f pos1, vtkVector2f pos2, double radius)
{
  auto dist = (pos2.GetX() - pos1.GetX()) * (pos2.GetX() - pos1.GetX()) +
    (pos2.GetY() - pos1.GetY()) * (pos2.GetY() - pos1.GetY());
  return dist < radius * radius;
}

//------------------------------------------------------------------------------
bool isNearLine(
  vtkVector2f p, vtkVector2f le1, vtkVector2f le2, double radius, double closestPoint[3] = nullptr)
{
  double p1[3] = { le1.GetX(), le1.GetY(), 0.0 };
  double p2[3] = { le2.GetX(), le2.GetY(), 0.0 };
  double xyz[3] = { p.GetX(), p.GetY(), 0.0 };
  double t;

  auto onLine = (vtkLine::DistanceToLine(xyz, p1, p2, t, closestPoint) <= radius * radius);
  bool res = { onLine && t < 1.0 && t > 0.0 };
  return res;
}
VTK_ABI_NAMESPACE_END
}

VTK_ABI_NAMESPACE_BEGIN
// using namespace equalizer;

class vtkEqualizerContextItem::vtkInternal
{
public:
  using EqualizerPoints = std::vector<equalizer::EqualizerPoint>;
  using ScopesRange = std::pair<int, int>;

  //------------------------------------------------------------------------------
  static std::vector<std::string> splitStringByDelimiter(const std::string& source, char delim)
  {
    std::stringstream ss(source);
    std::string item;
    std::vector<std::string> result;
    while (std::getline(ss, item, delim))
      result.push_back(std::move(item));

    return result;
  }

  //------------------------------------------------------------------------------
  void addPoint(const equalizer::EqualizerPoint& point)
  {
    this->Points.insert((std::lower_bound(this->Points.begin(), this->Points.end(), point)), point);
  }

  //------------------------------------------------------------------------------
  std::string pointsToString()
  {
    std::stringstream ss;
    for (auto point : this->Points)
    {
      ss << point.freq << "," << point.coef << ";";
    }

    return ss.str();
  }

  //------------------------------------------------------------------------------
  void setPoints(const std::string& str)
  {
    this->Points.clear();
    // TODO: refactoring: replace to common function
    // linked to https://gitlab.kitware.com/vtk/vtk/-/merge_requests/5930#note_926052
    std::vector<std::string> vecPointsStr{ splitStringByDelimiter(str, ';') };

    for (const auto& point : vecPointsStr)
    {
      std::vector<std::string> pointStr{ splitStringByDelimiter(point, ',') };
      if (pointStr.size() > 1)
      {
        float x = std::stof(pointStr.at(0));
        float y = std::stof(pointStr.at(1));
        this->Points.emplace_back(x, y);
      }
    }

    this->TakenPoint = -1;
  }

  //------------------------------------------------------------------------------
  ScopesRange GetScopes() const
  {
    auto left = 0;
    auto right = std::numeric_limits<int>::max();
    if (this->Points.size() < 2)
    {
      return ScopesRange(left, right);
    }

    if (this->TakenPoint == -1)
    {
      return ScopesRange(left, right);
    }

    // the first or last point
    if ((this->TakenPoint == 0) ||
      (this->TakenPoint == static_cast<vtkIdType>(this->Points.size()) - 1))
    {
      const equalizer::EqualizerPoint& point = this->Points.at(this->TakenPoint);
      left = point.freq;
      right = point.freq;
    }
    else
    {
      const equalizer::EqualizerPoint& pointLeft = this->Points.at(this->TakenPoint - 1);
      const equalizer::EqualizerPoint& pointRight = this->Points.at(this->TakenPoint + 1);
      left = pointLeft.freq;
      right = pointRight.freq;
    }

    return ScopesRange(left, right);
  }

  //------------------------------------------------------------------------------
  void LeftButtonPressEvent(const vtkVector2f& posScreen, vtkContextTransform* transform)
  {
    // 1.Try to find nearest point
    this->TakenPoint = -1;
    for (size_t i = 0; i < this->Points.size(); ++i)
    {
      auto point = transform->MapToScene(this->Points.at(i));
      if (equalizer::isNear(posScreen, point, equalizer::EqualizerPoint::radiusInteractive))
      {
        this->TakenPoint = static_cast<vtkIdType>(i);
        break;
      }
    }

    // 2.Try to find nearest line
    if (this->TakenPoint == -1)
    {
      auto itPrev = this->Points.begin();
      auto itCur = itPrev;

      for (++itCur; itCur != this->Points.cend(); ++itCur)
      {
        auto curPoint = transform->MapToScene(*itCur);
        auto prevPoint = transform->MapToScene(*itPrev);
        double closestPoint[3];
        if (equalizer::isNearLine(posScreen, prevPoint, curPoint,
              equalizer::EqualizerPoint::radiusInteractive, closestPoint))
        {
          vtkVector2f tmp(closestPoint[0], closestPoint[1]);
          auto inserted = this->Points.insert(itCur, transform->MapFromScene(tmp));
          this->TakenPoint = std::distance(this->Points.begin(), inserted);
          break;
        }
        itPrev = itCur;
      }
    }
  }

  //------------------------------------------------------------------------------
  bool RightButtonPressEvent(const vtkVector2f& posScreen, vtkContextTransform* transform)
  {
    if (this->Points.size() < 3)
    {
      return false;
    }

    for (auto it = this->Points.begin() + 1; it != this->Points.end() - 1; ++it)
    {
      auto point = transform->MapToScene(*it);
      if (equalizer::isNear(posScreen, point, equalizer::EqualizerPoint::radiusInteractive))
      {
        this->Points.erase(it);
        return true;
      }
    }

    return false;
  }

  //------------------------------------------------------------------------------
  bool Hit(const vtkVector2f& pos, vtkContextTransform* transform) const
  {
    for (const auto& point : this->Points)
    {
      auto scenePoint = transform->MapToScene(point);
      if (equalizer::isNear(pos, scenePoint, equalizer::EqualizerPoint::radiusInteractive))
      {
        return true;
      }
    }

    auto itPrev = this->Points.cbegin();
    auto itCur = itPrev;

    for (++itCur; itCur != this->Points.cend(); ++itCur)
    {
      auto curPoint = transform->MapToScene(*itCur);
      auto prevPoint = transform->MapToScene(*itPrev);
      if (equalizer::isNearLine(
            pos, prevPoint, curPoint, equalizer::EqualizerPoint::radiusInteractive))
      {
        return true;
      }
      itPrev = itCur;
    }

    return false;
  }

  // attributes
  EqualizerPoints Points;
  vtkIdType TakenPoint = -1;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkEqualizerContextItem);

//------------------------------------------------------------------------------
vtkEqualizerContextItem::vtkEqualizerContextItem()
  : Internal(new vtkInternal())
{
  this->Pen->SetColor(0, 0, 0);
  this->Pen->SetWidth(1.0);
  this->Pen->SetOpacityF(0.5);
  this->Brush->SetColor(0, 0, 0);
  this->Brush->SetOpacityF(0.5);
  this->Internal->addPoint(equalizer::EqualizerPoint(0, 1));
  this->Internal->addPoint(equalizer::EqualizerPoint(500, 1));
}

//------------------------------------------------------------------------------
vtkEqualizerContextItem::~vtkEqualizerContextItem()
{
  delete this->Internal;
}

//------------------------------------------------------------------------------
bool vtkEqualizerContextItem::Paint(vtkContext2D* painter)
{
  if (!this->Visible)
  {
    return false;
  }

  if (this->Internal->Points.size() < 2)
  {
    return false;
  }

  vtkContextScene* scene = this->GetScene();
  if (!scene)
  {
    return false;
  }

  if (!this->Transform)
  {
    return false;
  }

  painter->ApplyPen(this->Pen);
  painter->ApplyBrush(this->Brush);

  auto itPrev = this->Internal->Points.cbegin();
  auto itCur = itPrev;

  const equalizer::EqualizerPoint& firstPoint = this->Transform->MapToScene(*itCur);
  painter->DrawEllipse(firstPoint.freq, firstPoint.coef, equalizer::EqualizerPoint::radius,
    equalizer::EqualizerPoint::radius);
  for (++itCur; itCur != this->Internal->Points.cend(); ++itCur)
  {
    auto prevPoint = this->Transform->MapToScene(*itPrev);
    auto curPoint = this->Transform->MapToScene(*itCur);
    painter->DrawLine(prevPoint.GetX(), prevPoint.GetY(), curPoint.GetX(), curPoint.GetY());
    painter->DrawEllipse(curPoint.GetX(), curPoint.GetY(), equalizer::EqualizerPoint::radius,
      equalizer::EqualizerPoint::radius);
    itPrev = itCur;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkEqualizerContextItem::Hit(const vtkContextMouseEvent& mouse)
{
  if (!this->Transform)
  {
    return false;
  }

  auto hit = this->Internal->Hit(mouse.GetPos(), this->Transform);
  return this->Visible && hit;
}

//------------------------------------------------------------------------------
bool vtkEqualizerContextItem::MouseEnterEvent(const vtkContextMouseEvent& vtkNotUsed(mouse))
{
  return true;
}

//------------------------------------------------------------------------------
bool vtkEqualizerContextItem::MouseMoveEvent(const vtkContextMouseEvent& mouse)
{
  vtkContextScene* scene = this->GetScene();
  if (!scene || (this->MouseState != LEFT_BUTTON_PRESSED) || !this->Transform)
  {
    return false;
  }

  if (this->Internal->TakenPoint != -1)
  {
    equalizer::EqualizerPoint& point = this->Internal->Points.at(this->Internal->TakenPoint);
    auto scope = this->Internal->GetScopes();
    auto posScene = this->Transform->MapFromScene(mouse.GetPos());
    auto posX = vtkMath::ClampValue<int>(posScene.GetX(), scope.first, scope.second);

    auto posY = posScene.GetY();
    if (posY < 0)
    {
      posY = 0;
    }

    point.freq = posX;
    point.coef = posY;

    this->InvokeEvent(vtkCommand::InteractionEvent);
    this->GetScene()->SetDirty(true);
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkEqualizerContextItem::MouseLeaveEvent(const vtkContextMouseEvent& vtkNotUsed(mouse))
{
  return true;
}

//------------------------------------------------------------------------------
bool vtkEqualizerContextItem::MouseButtonPressEvent(const vtkContextMouseEvent& mouse)
{
  auto pos = mouse.GetPos();
  if (mouse.GetButton() == vtkContextMouseEvent::LEFT_BUTTON)
  {
    this->MouseState = LEFT_BUTTON_PRESSED;
    this->Internal->LeftButtonPressEvent(pos, this->Transform);
  }
  else if (mouse.GetButton() == vtkContextMouseEvent::RIGHT_BUTTON ||
    mouse.GetButton() == vtkContextMouseEvent::MIDDLE_BUTTON)
  {
    // middle click behave as right click
    this->MouseState = RIGHT_BUTTON_PRESSED;
    this->Internal->RightButtonPressEvent(pos, this->Transform);
  }

  this->InvokeEvent(vtkCommand::StartInteractionEvent);
  this->GetScene()->SetDirty(true);
  return true;
}

//------------------------------------------------------------------------------
bool vtkEqualizerContextItem::MouseButtonReleaseEvent(const vtkContextMouseEvent& vtkNotUsed(mouse))
{
  this->MouseState = NO_BUTTON;
  this->InvokeEvent(vtkCommand::EndInteractionEvent);
  this->GetScene()->SetDirty(true);
  return true;
}

//------------------------------------------------------------------------------
void vtkEqualizerContextItem::SetPoints(const std::string& points)
{
  this->Internal->setPoints(points);
}

//------------------------------------------------------------------------------
std::string vtkEqualizerContextItem::GetPoints() const
{
  return this->Internal->pointsToString();
}

//------------------------------------------------------------------------------
void vtkEqualizerContextItem::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << indent << "Mouse state = " << this->MouseState << endl;
  os << indent << "Points = " << this->Internal->pointsToString() << endl;
  os << indent << "Pen = " << this->Pen << endl;
  os << indent << "Brush = " << this->Brush << endl;
}
VTK_ABI_NAMESPACE_END
