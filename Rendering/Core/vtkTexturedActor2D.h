/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTexturedActor2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTexturedActor2D
 * @brief   actor that draws 2D data with texture support
 *
 * vtkTexturedActor2D is an Actor2D which has additional support for
 * textures, just like vtkActor. To use textures, the geometry must have
 * texture coordinates, and the texture must be set with SetTexture().
 *
 * @sa
 * vtkActor2D vtkProp vtkMapper2D vtkProperty2D
*/

#ifndef vtkTexturedActor2D_h
#define vtkTexturedActor2D_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkActor2D.h"

class vtkProp;
class vtkTexture;
class vtkViewport;
class vtkWindow;

class VTKRENDERINGCORE_EXPORT vtkTexturedActor2D : public vtkActor2D
{
public:
  static vtkTexturedActor2D* New();
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  vtkTypeMacro(vtkTexturedActor2D, vtkActor2D);

  //@{
  /**
   * Set/Get the texture object to control rendering texture maps.  This will
   * be a vtkTexture object. An actor does not need to have an associated
   * texture map and multiple actors can share one texture.
   */
  virtual void SetTexture(vtkTexture* texture);
  vtkGetObjectMacro(Texture, vtkTexture);
  //@}

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(vtkWindow* win) VTK_OVERRIDE;

  //@{
  /**
   * Support the standard render methods.
   */
  virtual int RenderOverlay(vtkViewport* viewport) VTK_OVERRIDE;
  virtual int RenderOpaqueGeometry(vtkViewport* viewport) VTK_OVERRIDE;
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport* viewport) VTK_OVERRIDE;
  //@}

  /**
   * Return this object's modified time.
   */
  virtual vtkMTimeType GetMTime() VTK_OVERRIDE;

  /**
   * Shallow copy of this vtkTexturedActor2D. Overrides vtkActor2D method.
   */
  virtual void ShallowCopy(vtkProp* prop) VTK_OVERRIDE;

protected:
  vtkTexturedActor2D();
  ~vtkTexturedActor2D();

  vtkTexture* Texture;

private:
  vtkTexturedActor2D(const vtkTexturedActor2D&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTexturedActor2D&) VTK_DELETE_FUNCTION;
};

#endif
