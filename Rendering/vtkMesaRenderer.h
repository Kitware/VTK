/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaRenderer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMesaRenderer - OpenGL renderer
// .SECTION Description
// vtkMesaRenderer is a concrete implementation of the abstract class
// vtkRenderer. vtkMesaRenderer interfaces to the mesa graphics library.
// This file is created, by a copy of vtkOpenGLRenderer

#ifndef __vtkMesaRenderer_h
#define __vtkMesaRenderer_h

#include "vtkRenderer.h"

class VTK_RENDERING_EXPORT vtkMesaRenderer : public vtkRenderer
{
protected:
  int NumberOfLightsBound;

public:
  static vtkMesaRenderer *New();
  vtkTypeMacro(vtkMesaRenderer,vtkRenderer);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Concrete open gl render method.
  void DeviceRender(void); 

  // Description:
  // Internal method temporarily removes lights before reloading them
  // into graphics pipeline.
  void ClearLights(void);

  void Clear(void);

  // Description:
  // Ask lights to load themselves into graphics pipeline.
  int UpdateLights(void);
  

  
  // Create a vtkMesaCamera, will be used by the super class
  // to create the correct camera object.
  virtual vtkCamera* MakeCamera();
  
  // Create a vtkMesaLight, will be used by the super class
  // to create the correct light object.
  virtual vtkLight* MakeLight();
  
protected:
  vtkMesaRenderer();
  ~vtkMesaRenderer();

  //BTX
  // Picking functions to be implemented by sub-classes
  virtual void DevicePickRender();
  virtual void StartPick(unsigned int pickFromSize);
  virtual void UpdatePickId();
  virtual void DonePick();
  virtual unsigned int GetPickedId();
  virtual unsigned int GetNumPickedIds();
  virtual int GetPickedIds(unsigned int atMost, unsigned int *callerBuffer);
  virtual double GetPickedZ();
  // Ivars used in picking
  class vtkGLPickInfo* PickInfo;
  //ETX
  double PickedZ;
private:
  vtkMesaRenderer(const vtkMesaRenderer&);  // Not implemented.
  void operator=(const vtkMesaRenderer&);  // Not implemented.
};

#endif
