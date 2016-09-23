/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextActor3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTextActor3D
 * @brief   An actor that displays text.
 *
 * The input text is rendered into a buffer, which in turn is used as a
 * texture applied onto a quad (a vtkImageActor is used under the hood).
 * @warning
 * This class is experimental at the moment.
 * - The orientation is not optimized, the quad should be oriented, not
 *   the text itself when it is rendered in the buffer (we end up with
 *   excessively big textures for 45 degrees angles).
 *   This will be fixed first.
 * - No checking is done at the moment regarding hardware texture size limits.
 *
 * @sa
 * vtkProp3D
*/

#ifndef vtkTextActor3D_h
#define vtkTextActor3D_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkProp3D.h"

class vtkImageActor;
class vtkImageData;
class vtkTextProperty;

class VTKRENDERINGCORE_EXPORT vtkTextActor3D : public vtkProp3D
{
public:
  static vtkTextActor3D *New();
  vtkTypeMacro(vtkTextActor3D,vtkProp3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Set the text string to be displayed.
   */
  vtkSetStringMacro(Input);
  vtkGetStringMacro(Input);
  //@}

  //@{
  /**
   * Set/Get the text property.
   */
  virtual void SetTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(TextProperty,vtkTextProperty);
  //@}

  /**
   * Since a 3D text actor is not pixel-aligned and positioned in 3D space,
   * the text is rendered at a constant DPI, rather than using the current
   * window DPI. This static method returns the DPI value used to produce the
   * text images.
   */
  static int GetRenderedDPI() { return 72; }

  /**
   * Shallow copy of this text actor. Overloads the virtual
   * vtkProp method.
   */
  void ShallowCopy(vtkProp *prop);

  /**
   * Get the bounds for this Prop3D as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
   */
  virtual double *GetBounds();
  void GetBounds(double bounds[6]) {this->vtkProp3D::GetBounds( bounds );}

  /**
   * Get the vtkTextRenderer-derived bounding box for the given vtkTextProperty
   * and text string str.  Results are returned in the four element bbox int
   * array.  This call can be used for sizing other elements.
   */
  int GetBoundingBox(int bbox[4]);

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS.
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(vtkWindow *);

  //@{
  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS.
   * Draw the text actor to the screen.
   */
  int RenderOpaqueGeometry(vtkViewport* viewport);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport* viewport);
  int RenderOverlay(vtkViewport* viewport);
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  virtual int HasTranslucentPolygonalGeometry();

protected:
   vtkTextActor3D();
  ~vtkTextActor3D();

  char            *Input;

  vtkImageActor   *ImageActor;
  vtkImageData    *ImageData;
  vtkTextProperty *TextProperty;

  vtkTimeStamp    BuildTime;

  virtual int UpdateImageActor();

private:
  vtkTextActor3D(const vtkTextActor3D&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTextActor3D&) VTK_DELETE_FUNCTION;
};


#endif
