// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLGL2PSHelper.h"

#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"

// Static allocation:
VTK_ABI_NAMESPACE_BEGIN
vtkOpenGLGL2PSHelper* vtkOpenGLGL2PSHelper::Instance = nullptr;

//------------------------------------------------------------------------------
vtkAbstractObjectFactoryNewMacro(vtkOpenGLGL2PSHelper);
vtkCxxSetObjectMacro(vtkOpenGLGL2PSHelper, RenderWindow, vtkRenderWindow);

//------------------------------------------------------------------------------
void vtkOpenGLGL2PSHelper::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkOpenGLGL2PSHelper* vtkOpenGLGL2PSHelper::GetInstance()
{
  return vtkOpenGLGL2PSHelper::Instance;
}

//------------------------------------------------------------------------------
void vtkOpenGLGL2PSHelper::SetInstance(vtkOpenGLGL2PSHelper* obj)
{
  if (obj == vtkOpenGLGL2PSHelper::Instance)
  {
    return;
  }

  if (vtkOpenGLGL2PSHelper::Instance)
  {
    vtkOpenGLGL2PSHelper::Instance->Delete();
  }

  if (obj)
  {
    obj->Register(nullptr);
  }

  vtkOpenGLGL2PSHelper::Instance = obj;
}

//------------------------------------------------------------------------------
vtkOpenGLGL2PSHelper::vtkOpenGLGL2PSHelper()
  : RenderWindow(nullptr)
  , ActiveState(Inactive)
  , TextAsPath(false)
  , PointSize(1.f)
  , LineWidth(1.f)
  , PointSizeFactor(5.f / 7.f)
  , LineWidthFactor(5.f / 7.f)
  , LineStipple(0xffff)
{
}

//------------------------------------------------------------------------------
vtkOpenGLGL2PSHelper::~vtkOpenGLGL2PSHelper()
{
  this->SetRenderWindow(nullptr);
}
VTK_ABI_NAMESPACE_END
