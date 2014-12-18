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
// .NAME vtkOpenGLLightMonitor -- A helper for painters that
// tracks state of OpenGL model-view and projection matrices.
//
// .SECTION Description:
// vtkOpenGLLightMonitor -- A helper for painters that
// tracks state of OpenGL lights. A Painter could use this
// to skip expensive processing that is only needed when
// lights change.
//
// this is not intended to be shared. each object should use it's
// own instance of this class. it's intended to be called once per
// render.

#ifndef vtkOpenGLLightMonitor_h
#define vtkOpenGLLightMonitor_h

#include "vtkRenderingOpenGLModule.h" // for export macro
#include "vtkObject.h"

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLLightMonitor : public vtkObject
{
public:
  static vtkOpenGLLightMonitor* New();
  static vtkOpenGLLightMonitor *New(int lightId);
  vtkTypeMacro(vtkOpenGLLightMonitor, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the light id of the OpenGL light to track. The
  // light id must be set prior to use. Default value 0.
  vtkSetMacro(LightId, int);
  vtkGetMacro(LightId, int);

  // Description:
  // Fetches the current GL state and updates the
  // internal copies of the data. returns true if
  // any of the tracked OpenGL lights have changed.
  // Typically this is the only function a user needs
  // to call.
  bool StateChanged();

  // Description:
  // Fetch and save OpenGL light state. Note,
  // this is done automatically in SateChanged.
  void Update();

  //BTX
  // Description:
  // Setters for internal state.
  void SetEnabled(int val);
  void SetAmbient(float *val);
  void SetDiffuse(float *val);
  void SetSpecular(float *val);
  void SetPosition(float *val);
  void SetSpotDirection(float *val);
  void SetSpotExponent(float val);
  void SetSpotCutoff(float val);
  void SetAttenuation(float *val);
  //ETX

private:
  vtkOpenGLLightMonitor(int lightId) : LightId(lightId), UpTime(0)
  { this->Initialize(); }

  vtkOpenGLLightMonitor() : LightId(0), UpTime(0)
  { this->Initialize(); }

  ~vtkOpenGLLightMonitor(){}

  void Initialize();

private:
  int LightId;
  int Enabled;
  float Ambient[4];
  float Diffuse[4];
  float Specular[4];
  float Position[4];
  float SpotDirection[3];
  float SpotExponent;
  float SpotCutoff;
  float Attenuation[3];
  long long UpTime;

private:
  vtkOpenGLLightMonitor(const vtkOpenGLLightMonitor &); // Not implemented
  void operator=(const vtkOpenGLLightMonitor &); // Not implemented
};

#endif
