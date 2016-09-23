/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLModelViewProjectionMonitor

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLModelViewProjectionMonitor.h"
#include "vtkObjectFactory.h"
#include "vtkgl.h"
#include <cstring>
#include <cmath>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLModelViewProjectionMonitor)

//-----------------------------------------------------------------------------
void vtkOpenGLModelViewProjectionMonitor::Initialize()
{
  memset(this->Projection,0,16*sizeof(float));
  memset(this->ModelView,0,16*sizeof(float));
}

//-----------------------------------------------------------------------------
#define vtkOpenGLModelViewProjectionMonitorSetVectorMacro(_name, _n) \
void vtkOpenGLModelViewProjectionMonitor::Set##_name(float *val)     \
{                                                    \
  int changed = 0;                                   \
  for (int i=0; i<_n; ++i)                           \
  {                                                \
    if ( fabs( val[i] - this->_name[i] ) > 1e-5f )   \
    {                                              \
      changed=1;                                     \
      this->_name[i] = val[i];                       \
    }                                              \
  }                                                \
  if ( changed )                                     \
  {                                                \
    this->UpTime += 1;                               \
  }                                                \
}
vtkOpenGLModelViewProjectionMonitorSetVectorMacro(Projection, 16)
vtkOpenGLModelViewProjectionMonitorSetVectorMacro(ModelView, 16)

//-----------------------------------------------------------------------------
void vtkOpenGLModelViewProjectionMonitor::Update()
{
  GLfloat matrix[16];
  glGetFloatv(GL_PROJECTION_MATRIX, matrix);
  this->SetProjection(matrix);

  glGetFloatv(GL_MODELVIEW_MATRIX,  matrix);
  this->SetModelView(matrix);
}

//-----------------------------------------------------------------------------
bool vtkOpenGLModelViewProjectionMonitor::StateChanged()
{
  long long oldUpTime = this->UpTime;
  this->Update();
  return (this->UpTime != oldUpTime);
}

//-----------------------------------------------------------------------------
void vtkOpenGLModelViewProjectionMonitor::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Projection=";
  for (int q=0; q<16; ++q)
  {
    os << this->Projection[q] << " ";
  }
  os << endl;
  os << indent << "ModelView=";
  for (int q=0; q<16; ++q)
  {
    os << this->ModelView[q] << " ";
  }
  os << endl;
  os << indent << "UpTime=" << this->UpTime << endl;
}
