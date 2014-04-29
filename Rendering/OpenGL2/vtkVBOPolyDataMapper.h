/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVBOPolyDataMapper - PolyDataMapper using VBOs primarily to render.
// .SECTION Description
// PolyDataMapper that uses a VBOs to do the actual rendering.

#ifndef __vtkVBOPolyDataMapper_h
#define __vtkVBOPolyDataMapper_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkPolyDataMapper.h"

class VTKRENDERINGOPENGL2_EXPORT vtkVBOPolyDataMapper : public vtkPolyDataMapper
{
public:
  static vtkVBOPolyDataMapper* New();
  vtkTypeMacro(vtkVBOPolyDataMapper, vtkPolyDataMapper)
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implemented by sub classes. Actual rendering is done here.
  virtual void RenderPiece(vtkRenderer *ren, vtkActor *act);

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release. Merely propagates the call to the painter.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Used by vtkHardwareSelector to determine if the prop supports hardware
  // selection.
  virtual bool GetSupportsSelection() { return false; }

  // Description:
  // Returns if the mapper does not expect to have translucent geometry. This
  // may happen when using ScalarMode is set to not map scalars i.e. render the
  // scalar array directly as colors and the scalar array has opacity i.e. alpha
  // component. Note that even if this method returns true, an actor may treat
  // the geometry as translucent since a constant translucency is set on the
  // property, for example.
  // Overridden to use the actual data and ScalarMode to determine if we have
  // opaque geometry.
  virtual bool GetIsOpaque();

protected:
  vtkVBOPolyDataMapper();
  ~vtkVBOPolyDataMapper();

  // Description:
  // Called in GetBounds(). When this method is called, the consider the input
  // to be updated depending on whether this->Static is set or not. This method
  // simply obtains the bounds from the data-object and returns it.
  virtual void ComputeBounds();

  // Description:
  // Determine what shader to use and compile/link it
  virtual void UpdateShader(vtkRenderer *ren, vtkActor *act);

  // Description:
  // Update the scene when necessary.
  void UpdateVBO(vtkActor *act);

  void MapScalars(vtkDataSet* output, double alpha, bool multiplyWithAlpha,
                  vtkDataSet* input);

  // Description:
  // Set the shader parameteres related to lighting
  void SetLightingShaderParameters(vtkRenderer *ren, vtkActor *act);

  // Description:
  // Set the shader parameteres related to lighting
  void SetCameraShaderParameters(vtkRenderer *ren, vtkActor *act);

  // Description:
  // Set the shader parameteres related to lighting
  void SetPropertyShaderParameters(vtkRenderer *ren, vtkActor *act);

  // Description:
  // The scene used by the mapper, all rendering is deferred to the scene.
  class Private;
  Private *Internal;

  bool UsingScalarColoring;
  vtkTimeStamp VBOUpdateTime; // When was the VBO updated?
  bool Initialized; // Hack - ensure glewinit has been called - move to window.

private:
  vtkVBOPolyDataMapper(const vtkVBOPolyDataMapper&); // Not implemented.
  void operator=(const vtkVBOPolyDataMapper&); // Not implemented.
};

#endif
