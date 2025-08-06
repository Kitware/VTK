// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "Private/vtkEGLConfig.h"

//------------------------------------------------------------------------------
void vtkEGLConfig::SetOnscreenRendering(bool onscreenRendering)
{
  this->OnscreenRendering = onscreenRendering;
};
