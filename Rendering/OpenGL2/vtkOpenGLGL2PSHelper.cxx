/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLGL2PSHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLGL2PSHelper.h"

#include "vtkObjectFactory.h"

// Static allocation:
vtkOpenGLGL2PSHelper *vtkOpenGLGL2PSHelper::Instance = NULL;

//------------------------------------------------------------------------------
vtkAbstractObjectFactoryNewMacro(vtkOpenGLGL2PSHelper)

//------------------------------------------------------------------------------
void vtkOpenGLGL2PSHelper::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkOpenGLGL2PSHelper *vtkOpenGLGL2PSHelper::GetInstance()
{
  return vtkOpenGLGL2PSHelper::Instance;
}

//------------------------------------------------------------------------------
void vtkOpenGLGL2PSHelper::SetInstance(vtkOpenGLGL2PSHelper *obj)
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
    obj->Register(NULL);
  }

  vtkOpenGLGL2PSHelper::Instance = obj;
}

//------------------------------------------------------------------------------
vtkOpenGLGL2PSHelper::vtkOpenGLGL2PSHelper()
  : RenderWindow(NULL),
    ActiveState(Inactive),
    TextAsPath(false),
    PointSize(1.f),
    LineWidth(1.f),
    PointSizeFactor(5.f / 7.f),
    LineWidthFactor(5.f / 7.f),
    LineStipple(0xffff)
{
}

//------------------------------------------------------------------------------
vtkOpenGLGL2PSHelper::~vtkOpenGLGL2PSHelper()
{
}
