// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkToneMappingPass.h"
#include "vtkObjectFactory.h"

#include <algorithm>
#include <cassert>
#include <cmath>

VTK_ABI_NAMESPACE_BEGIN
vtkObjectFactoryNewMacro(vtkToneMappingPass);
vtkCxxSetObjectMacro(vtkToneMappingPass, DelegatePass, vtkRenderPass);

//------------------------------------------------------------------------------
vtkToneMappingPass::~vtkToneMappingPass()
{
  if (this->DelegatePass != nullptr)
  {
    this->DelegatePass->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkToneMappingPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ToneMappingType: " << this->ToneMappingType << endl;
  os << indent << "Exposure: " << this->Exposure << endl;
  os << indent << "Contrast: " << this->Contrast << endl;
  os << indent << "Shoulder: " << this->Shoulder << endl;
  os << indent << "MidIn: " << this->MidIn << endl;
  os << indent << "MidOut: " << this->MidOut << endl;
  os << indent << "HdrMax: " << this->HdrMax << endl;
  os << indent << "UseACES: " << this->UseACES << endl;
  os << indent << "DelegatePass:";
  if (this->DelegatePass != nullptr)
  {
    this->DelegatePass->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
}

//------------------------------------------------------------------------------
void vtkToneMappingPass::ReleaseGraphicsResources(vtkWindow* w)
{
  assert("pre: w_exists" && w != nullptr);
  if (this->DelegatePass != nullptr)
  {
    this->DelegatePass->ReleaseGraphicsResources(w);
  }
}

//------------------------------------------------------------------------------
void vtkToneMappingPass::SetGenericFilmicDefaultPresets()
{
  this->Contrast = 1.6773;
  this->Shoulder = 0.9714;
  this->MidIn = 0.18;
  this->MidOut = 0.18;
  this->HdrMax = 11.0785;
  this->UseACES = true;

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkToneMappingPass::SetGenericFilmicUncharted2Presets()
{
  this->Contrast = 1.1759;
  this->Shoulder = 0.9746;
  this->MidIn = 0.18;
  this->MidOut = 0.18;
  this->HdrMax = 6.3704;
  this->UseACES = false;

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkToneMappingPass::UpdateAnchorCurveGenericFilmic()
{
  if (this->PreComputeMTime < this->GetMTime())
  {
    this->PreComputeAnchorCurveGenericFilmic();
    this->PreComputeMTime = this->GetMTime();
  }
}

//------------------------------------------------------------------------------
void vtkToneMappingPass::PreComputeAnchorCurveGenericFilmic()
{
  const float& a = this->Contrast;
  const float& d = this->Shoulder;
  const float& m = this->MidIn;
  const float& n = this->MidOut;

  // Pre compute shape of the curve parameters
  this->ClippingPoint =
    -((powf(m, -a * d) *
        (-powf(m, a) +
          (n *
            (powf(m, a * d) * n * powf(this->HdrMax, a) - powf(m, a) * powf(this->HdrMax, a * d))) /
            (powf(m, a * d) * n - n * powf(this->HdrMax, a * d)))) /
      n);

  // Avoid discontinuous curve by clamping to 0
  this->ToeSpeed =
    std::max((powf(m, a * d) * n * powf(this->HdrMax, a) - powf(m, a) * powf(this->HdrMax, a * d)) /
        (powf(m, a * d) * n - n * powf(this->HdrMax, a * d)),
      0.f);
}
VTK_ABI_NAMESPACE_END
