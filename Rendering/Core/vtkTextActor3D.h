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
// .NAME vtkTextActor3D - An actor that displays text.
// .SECTION Description
// The input text is rendered into a buffer, which in turn is used as a
// texture applied onto a quad (a vtkImageActor is used under the hood).
// .SECTION Warning
// This class is experimental at the moment.
// - The orientation is not optimized, the quad should be oriented, not
//   the text itself when it is rendered in the buffer (we end up with
//   excessively big textures for 45 degrees angles).
//   This will be fixed first.
// - No checking is done at the moment regarding hardware texture size limits.
//
// .SECTION See Also
// vtkProp3D

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

  // Description:
  // Set the text string to be displayed.
  vtkSetStringMacro(Input);
  vtkGetStringMacro(Input);

  // Description:
  // Set/Get the text property.
  virtual void SetTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(TextProperty,vtkTextProperty);

  // Description:
  // Shallow copy of this text actor. Overloads the virtual
  // vtkProp method.
  void ShallowCopy(vtkProp *prop);

  // Description:
  // Get the bounds for this Prop3D as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
  virtual double *GetBounds();
  void GetBounds(double bounds[6]) {this->vtkProp3D::GetBounds( bounds );}

  // Description:
  // Get the Freetype-derived real bounding box for the given vtkTextProperty
  // and text string str.  Results are returned in the four element bbox int
  // array.  This call can be used for sizing other elements.
  virtual int GetBoundingBox(int bbox[4]);

  //BTX
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS.
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS.
  // Draw the text actor to the screen.
  int RenderOpaqueGeometry(vtkViewport* viewport);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport* viewport);
  int RenderOverlay(vtkViewport* viewport);

  // Description:
  // Does this prop have some translucent polygonal geometry?
  virtual int HasTranslucentPolygonalGeometry();
  //ETX

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
  vtkTextActor3D(const vtkTextActor3D&);  // Not implemented.
  void operator=(const vtkTextActor3D&);  // Not implemented.
};


#endif
