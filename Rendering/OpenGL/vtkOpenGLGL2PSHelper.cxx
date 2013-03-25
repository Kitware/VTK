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

bool vtkOpenGLGL2PSHelper::InGL2PSRender = false;
GLfloat vtkOpenGLGL2PSHelper::PointSizeFactor = 0.f;
GLfloat vtkOpenGLGL2PSHelper::LineWidthFactor = 0.f;
GLfloat vtkOpenGLGL2PSHelper::PointSizeToken = 0;
GLfloat vtkOpenGLGL2PSHelper::LineWidthToken = 0;
GLfloat vtkOpenGLGL2PSHelper::StippleBeginToken = 0;
GLfloat vtkOpenGLGL2PSHelper::StippleEndToken = 0;
