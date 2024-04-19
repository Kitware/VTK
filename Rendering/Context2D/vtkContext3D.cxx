// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkContext3D.h"
#include "vtkContextDevice3D.h"
#include "vtkTransform.h"

#include "vtkObjectFactory.h"

#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkContext3D);

void vtkContext3D::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << indent << "Context Device: ";
  if (this->Device)
  {
    os << endl;
    this->Device->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }
}

bool vtkContext3D::Begin(vtkContextDevice3D* device)
{
  if (this->Device == device)
  {
    return true;
  }
  this->Device = device;
  return true;
}

vtkContextDevice3D* vtkContext3D::GetDevice()
{
  return this->Device;
}

bool vtkContext3D::End()
{
  if (this->Device)
  {
    this->Device = nullptr;
  }
  return true;
}

void vtkContext3D::DrawLine(const vtkVector3f& start, const vtkVector3f& end)
{
  assert(this->Device);
  vtkVector3f line[2] = { start, end };
  this->Device->DrawPoly(line[0].GetData(), 2);
}

void vtkContext3D::DrawPoly(const float* points, int n)
{
  assert(this->Device);
  this->Device->DrawPoly(points, n);
}

void vtkContext3D::DrawPoint(const vtkVector3f& point)
{
  assert(this->Device);
  this->Device->DrawPoints(point.GetData(), 1);
}

void vtkContext3D::DrawPoints(const float* points, int n)
{
  assert(this->Device);
  this->Device->DrawPoints(points, n);
}

void vtkContext3D::DrawPoints(const float* points, int n, unsigned char* colors, int nc_comps)
{
  assert(this->Device);
  this->Device->DrawPoints(points, n, colors, nc_comps);
}

void vtkContext3D::DrawPoints(
  vtkDataArray* positions, vtkUnsignedCharArray* colors, std::uintptr_t cacheIdentifier)
{
  assert(this->Device);
  this->Device->DrawPoints(positions, colors, cacheIdentifier);
}

void vtkContext3D::DrawTriangleMesh(const float* mesh, int n, const unsigned char* colors, int nc)
{
  assert(this->Device);
  this->Device->DrawTriangleMesh(mesh, n, colors, nc);
}

void vtkContext3D::DrawTriangleMesh(
  vtkDataArray* positions, vtkUnsignedCharArray* colors, std::uintptr_t cacheIdentifier)
{
  assert(this->Device);
  this->Device->DrawTriangleMesh(positions, colors, cacheIdentifier);
}

void vtkContext3D::ApplyPen(vtkPen* pen)
{
  assert(this->Device);
  this->Device->ApplyPen(pen);
}

void vtkContext3D::ApplyBrush(vtkBrush* brush)
{
  assert(this->Device);
  this->Device->ApplyBrush(brush);
}

void vtkContext3D::SetTransform(vtkTransform* transform)
{
  if (transform)
  {
    this->Device->SetMatrix(transform->GetMatrix());
  }
}

vtkTransform* vtkContext3D::GetTransform()
{
  if (this->Device && this->Transform)
  {
    this->Device->GetMatrix(this->Transform->GetMatrix());
    return this->Transform;
  }
  return nullptr;
}

void vtkContext3D::AppendTransform(vtkTransform* transform)
{
  if (transform)
  {
    this->Device->MultiplyMatrix(transform->GetMatrix());
  }
}

void vtkContext3D::PushMatrix()
{
  this->Device->PushMatrix();
}

void vtkContext3D::PopMatrix()
{
  this->Device->PopMatrix();
}

void vtkContext3D::EnableClippingPlane(int i, double* planeEquation)
{
  assert(this->Device);
  this->Device->EnableClippingPlane(i, planeEquation);
}

void vtkContext3D::DisableClippingPlane(int i)
{
  assert(this->Device);
  this->Device->DisableClippingPlane(i);
}

vtkContext3D::vtkContext3D() = default;

vtkContext3D::~vtkContext3D() = default;
VTK_ABI_NAMESPACE_END
