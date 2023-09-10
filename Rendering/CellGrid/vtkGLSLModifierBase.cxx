// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGLSLModifierBase.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkLogger.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkInformationKeyMacro(vtkGLSLModifierBase, GLSL_MODIFIERS, ObjectBase);

//------------------------------------------------------------------------------
vtkGLSLModifierBase::vtkGLSLModifierBase() = default;

//------------------------------------------------------------------------------
vtkGLSLModifierBase::~vtkGLSLModifierBase() = default;

//------------------------------------------------------------------------------
void vtkGLSLModifierBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

VTK_ABI_NAMESPACE_END
