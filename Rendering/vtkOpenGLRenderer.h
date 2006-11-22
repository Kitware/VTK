/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRenderer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLRenderer - OpenGL renderer
// .SECTION Description
// vtkOpenGLRenderer is a concrete implementation of the abstract class
// vtkRenderer. vtkOpenGLRenderer interfaces to the OpenGL graphics library.

#ifndef __vtkOpenGLRenderer_h
#define __vtkOpenGLRenderer_h

#include "vtkRenderer.h"

class vtkOpenGLRendererLayerList; // Pimpl

class VTK_RENDERING_EXPORT vtkOpenGLRenderer : public vtkRenderer
{
protected:
  int NumberOfLightsBound;

public:
  static vtkOpenGLRenderer *New();
  vtkTypeRevisionMacro(vtkOpenGLRenderer,vtkRenderer);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Concrete open gl render method.
  void DeviceRender(void);
  
  // Description:
  // Render translucent geometry. Default implementation just call
  // UpdateTranslucentGeometry().
  // Subclasses of vtkRenderer that can deal with depth peeling must
  // override this method.
  virtual void DeviceRenderTranslucentGeometry();
  
  // Description:
  // Internal method temporarily removes lights before reloading them
  // into graphics pipeline.
  void ClearLights(void);

  void Clear(void);

  // Description:
  // Ask lights to load themselves into graphics pipeline.
  int UpdateLights(void);
  
protected:
  vtkOpenGLRenderer();
  ~vtkOpenGLRenderer();

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
 
  // Description:
  // Render a peel layer. If there is no more GPU RAM to save the texture,
  // return false otherwise returns true. Also if layer==0 and no prop have
  // been rendered (there is no translucent geometry), it returns false.
  // \pre positive_layer: layer>=0
  int RenderPeel(int layer);
  
  // Description:
  // This flag is on if the current OpenGL context supports extensions
  // required by the depth peeling technique.
  int DepthPeelingIsSupported;
  
  // Description:
  // This flag is on once the OpenGL extensions required by the depth peeling
  // technique have been checked.
  int DepthPeelingIsSupportedChecked;
  
  // Description:
  // Used by the depth peeling technique to store the transparency layers.
  vtkOpenGLRendererLayerList *LayerList;
  
  unsigned int OpaqueLayerZ;
  unsigned int TransparentLayerZ;
  unsigned int ProgramShader;
  
  // Description:
  // Cache viewport values for depth peeling.
  int ViewportX;
  int ViewportY;
  int ViewportWidth;
  int ViewportHeight;
  
  // Description:
  // Actual depth format: vtkgl::DEPTH_COMPONENT16_ARB
  // or vtkgl::DEPTH_COMPONENT24_ARB
  unsigned int DepthFormat;
  
private:
  vtkOpenGLRenderer(const vtkOpenGLRenderer&);  // Not implemented.
  void operator=(const vtkOpenGLRenderer&);  // Not implemented.
};

#endif
