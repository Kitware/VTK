// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkContextClip.h"
#include "vtkContext2D.h"
#include "vtkContextDevice2D.h"
#include "vtkContextScenePrivate.h"
#include "vtkObjectFactory.h"
#include "vtkTransform2D.h"
#include "vtkVector.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkContextClip);

//------------------------------------------------------------------------------
vtkContextClip::vtkContextClip()
{
  this->Dims[0] = 0.0;
  this->Dims[1] = 0.0;
  this->Dims[2] = 100.0;
  this->Dims[3] = 100.0;
}

//------------------------------------------------------------------------------
vtkContextClip::~vtkContextClip() = default;

//------------------------------------------------------------------------------
bool vtkContextClip::Paint(vtkContext2D* painter)
{
  // Clip rendering for all child items.
  // Check whether the scene has a transform - use it if so
  float* clipBy = this->Dims;

  int clipi[] = { vtkContext2D::FloatToInt(clipBy[0]), vtkContext2D::FloatToInt(clipBy[1]),
    vtkContext2D::FloatToInt(clipBy[2]), vtkContext2D::FloatToInt(clipBy[3]) };

  painter->GetDevice()->SetClipping(clipi);
  painter->GetDevice()->EnableClipping(true);
  bool result = this->PaintChildren(painter);
  painter->GetDevice()->EnableClipping(false);
  return result;
}

//------------------------------------------------------------------------------
void vtkContextClip::Update() {}

//------------------------------------------------------------------------------
void vtkContextClip::SetClip(float x, float y, float width, float height)
{
  this->Dims[0] = x;
  this->Dims[1] = y;
  // assure stored width, height are >= 0
  this->Dims[2] = std::max<float>(0.0, width);
  this->Dims[3] = std::max<float>(0.0, height);
}

//------------------------------------------------------------------------------
void vtkContextClip::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
