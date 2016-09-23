/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLLightMonitor

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLLightMonitor.h"
#include "vtkObjectFactory.h"
#include "vtkgl.h"
#include <cstring>
#include <cmath>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLLightMonitor)

//-----------------------------------------------------------------------------
vtkOpenGLLightMonitor *vtkOpenGLLightMonitor::New(int lightId)
{
  vtkOpenGLLightMonitor *mon = vtkOpenGLLightMonitor::New();
  mon->SetLightId(lightId);
  return mon;
}

//-----------------------------------------------------------------------------
void vtkOpenGLLightMonitor::Initialize()
{
  this->Enabled = 0;
  memset(this->Ambient,0,4*sizeof(float));
  memset(this->Diffuse,0,4*sizeof(float));
  memset(this->Specular,0,4*sizeof(float));
  memset(this->Position,0,4*sizeof(float));
  memset(this->SpotDirection,0,3*sizeof(float));
  this->SpotExponent = 0.0f;
  this->SpotCutoff = 0.0f;
  memset(this->Attenuation,0,3*sizeof(float));
  this->UpTime = 0;
}

//-----------------------------------------------------------------------------
void vtkOpenGLLightMonitor::SetEnabled(int val)
{
  if (this->Enabled == val)
  {
    return;
  }
  this->Enabled = val;
  this->UpTime += 1;
}

//-----------------------------------------------------------------------------
#define vtkOpenGLLightMonitorSetMacro(_name)      \
void vtkOpenGLLightMonitor::Set##_name(float val) \
{                                           \
  if ( fabs ( this->_name - val ) < 1e-5f ) \
  {                                       \
    return;                                 \
  }                                       \
  this->_name = val;                        \
  this->UpTime += 1;                        \
}
vtkOpenGLLightMonitorSetMacro(SpotExponent)
vtkOpenGLLightMonitorSetMacro(SpotCutoff)

//-----------------------------------------------------------------------------
#define vtkOpenGLLightMonitorSetVectorMacro(_name, _n)   \
void vtkOpenGLLightMonitor::Set##_name(float *val)       \
{                                                  \
  int changed = 0;                                 \
  for (int i=0; i<_n; ++i)                         \
  {                                              \
    if ( fabs( val[i] - this->_name[i] ) > 1e-5f ) \
    {                                            \
      changed=1;                                   \
      this->_name[i] = val[i];                     \
    }                                            \
  }                                              \
  if ( changed )                                   \
  {                                              \
    this->UpTime += 1;                             \
  }                                              \
}
vtkOpenGLLightMonitorSetVectorMacro(Ambient, 4)
vtkOpenGLLightMonitorSetVectorMacro(Diffuse, 4)
vtkOpenGLLightMonitorSetVectorMacro(Specular, 4)
vtkOpenGLLightMonitorSetVectorMacro(Position, 4)
vtkOpenGLLightMonitorSetVectorMacro(SpotDirection, 3)
vtkOpenGLLightMonitorSetVectorMacro(Attenuation, 3)

//-----------------------------------------------------------------------------
void vtkOpenGLLightMonitor::Update()
{
  float param[4];
  GLenum light = (GLenum)GL_LIGHT0+this->LightId;

  if (glIsEnabled(light))
  {
    this->SetEnabled(1);
  }
  else
  {
    this->SetEnabled(0);
  }

  if (this->Enabled)
  {
    glGetLightfv(light, GL_AMBIENT, param);
    this->SetAmbient(param);

    glGetLightfv(light, GL_DIFFUSE, param);
    this->SetDiffuse(param);

    glGetLightfv(light, GL_SPECULAR, param);
    this->SetSpecular(param);

    glGetLightfv(light, GL_POSITION, param);
    this->SetPosition(param);

    glGetLightfv(light, GL_SPOT_DIRECTION, param);
    this->SetSpotDirection(param);

    glGetLightfv(light, GL_SPOT_EXPONENT, param);
    this->SetSpotExponent(param[0]);

    glGetLightfv(light, GL_SPOT_CUTOFF, param);
    this->SetSpotCutoff(param[0]);

    glGetLightfv(light, GL_CONSTANT_ATTENUATION, param);
    glGetLightfv(light, GL_LINEAR_ATTENUATION, param+1);
    glGetLightfv(light, GL_QUADRATIC_ATTENUATION, param+2);
    this->SetAttenuation(param);
  }
}

//-----------------------------------------------------------------------------
bool vtkOpenGLLightMonitor::StateChanged()
{
  if (!glIsEnabled(GL_LIGHTING))
  {
    return false;
  }
  long long lastUpTime = this->UpTime;
  this->Update();
  return (lastUpTime != this->UpTime);
}

//-----------------------------------------------------------------------------
void vtkOpenGLLightMonitor::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "LightId=" << this->LightId << endl;
  os << indent << "Enabled=" << this->Enabled << endl;
  os << indent << "Ambient=";
  for (int q=0; q<4; ++q)
  {
    os << indent << this->Ambient[q] << " ";
  }
  os << endl;
  os << indent << "Diffuse=";
  for (int q=0; q<4; ++q)
  {
    os << indent << this->Diffuse[q] << " ";
  }
  os << endl;
  os << indent << "Specular=";
  for (int q=0; q<4; ++q)
  {
    os << this->Specular[q] << " ";
  }
  os << endl;
  os << indent << "Position=";
  for (int q=0; q<4; ++q)
  {
    os << this->Position[q] << " ";
  }
  os << endl;
  os << indent << "SpotDirection=";
  for (int q=0; q<3; ++q)
  {
    os << this->SpotDirection[q] << " ";
  }
  os << endl;
  os << indent << "SpotExponent=" << this->SpotExponent << endl;
  os << indent << "SpotCutoff=" << this->SpotCutoff << endl;
  os << indent << "Attenuation=";
  for (int q=0; q<3; ++q)
  {
    os << indent << this->Attenuation[q] << " ";
  }
  os << endl;
  os << indent << "UpTime=" << this->UpTime << endl;
}
