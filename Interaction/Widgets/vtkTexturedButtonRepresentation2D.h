/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTexturedButtonRepresentation2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTexturedButtonRepresentation2D - defines a representation for a vtkButtonWidget
// .SECTION Description
// This class implements one type of vtkButtonRepresentation. It changes the
// appearance of a user-provided polydata by assigning textures according to
// the current button state. It also provides highlighting (when hovering and
// selecting the button) by fiddling with the actor's property. Since this is
// a 2D version, the button is rendered in the overlay plane. Typically it is
// positioned in display coordinates, but it can be anchored to a world
// position so it will appear to move as the camera moves.
//
// To use this representation, always begin by specifying the number of
// button states.  Then provide a polydata (the polydata should have associated
// texture coordinates), and a list of textures cooresponding to the button
// states. Optionally, the HoveringProperty and SelectionProperty can be
// adjusted to obtain the appropriate appearance.

// ,SECTION Caveats
// There are two variants of the PlaceWidget() method. The first PlaceWidget(bds[6])
// allows the widget to be placed in the display coordinates fixed to the overlay
// plane. The second PlaceWidget(anchor[3],size[2]) places the widget in world space;
// hence it will appear to move as the camera moves around the scene.

// .SECTION See Also
// vtkButtonWidget vtkButtonRepresentation vtkTexturedButtonRepresentation
// vtkProp3DButtonRepresentation


#ifndef __vtkTexturedButtonRepresentation2D_h
#define __vtkTexturedButtonRepresentation2D_h

#include "vtkButtonRepresentation.h"

class vtkProperty2D;
class vtkImageData;
class vtkTextureArray; //PIMPLd
class vtkProperty2D;
class vtkAlgorithmOutput;
class vtkBalloonRepresentation;
class vtkCoordinate;


class VTK_WIDGETS_EXPORT vtkTexturedButtonRepresentation2D : public vtkButtonRepresentation
{
public:
  // Description:
  // Instantiate the class.
  static vtkTexturedButtonRepresentation2D *New();

  // Description:
  // Standard methods for the class.
  vtkTypeMacro(vtkTexturedButtonRepresentation2D,vtkButtonRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the property to use when the button is to appear "normal"
  // i.e., the mouse pointer is not hovering or selecting the button.
  virtual void SetProperty(vtkProperty2D *p);
  vtkGetObjectMacro(Property,vtkProperty2D);

  // Description:
  // Specify the property to use when the hovering over the button.
  virtual void SetHoveringProperty(vtkProperty2D *p);
  vtkGetObjectMacro(HoveringProperty,vtkProperty2D);

  // Description:
  // Specify the property to use when selecting the button.
  virtual void SetSelectingProperty(vtkProperty2D *p);
  vtkGetObjectMacro(SelectingProperty,vtkProperty2D);

  // Description:
  // Add the ith texture corresponding to the ith button state.
  // The parameter i should be 0<=i<NumberOfStates.
  void SetButtonTexture(int i, vtkImageData *image);
  vtkImageData *GetButtonTexture(int i);

  // Description:
  // Grab the underlying vtkBalloonRepresentation used to position and display
  // the button texture.
  vtkBalloonRepresentation *GetBalloon() {return this->Balloon;}

  // Description:
  // Provide the necessary methods to satisfy the vtkWidgetRepresentation API.
  virtual int ComputeInteractionState(int X, int Y, int modify=0);
  virtual void BuildRepresentation();
  virtual void Highlight(int state);

  // Description:
  // Conventional PlaceWidget() method to satisfy the vtkWidgetRepresentation API.
  // In this version, bounds[6] specifies a rectangle in *display* coordinates
  // in which to place the button. The values for bounds[4] and bounds[5] can be
  // set to zero. Note that PlaceWidget() is typically called at the end of configuring
  // the button representation.
  virtual void PlaceWidget(double bounds[6]);

  // Description:
  // This alternative PlaceWidget() method can be used to anchor the button
  // to a 3D point. In this case, the button representation will move around
  // the screen as the camera moves around the world space. The first
  // parameter anchor[3] is the world point anchor position (attached to the
  // lower left portion of the button by default); and the size[2] parameter
  // defines a x-y box in display coordinates in which the button will
  // fit. Note that you can grab the vtkBalloonRepresentation and set an
  // offset value if the anchor point is to be elsewhere on the button.
  virtual void PlaceWidget(double anchor[3], int size[2]);

  // Description:
  // Provide the necessary methods to satisfy the rendering API.
  virtual void ShallowCopy(vtkProp *prop);
  virtual double *GetBounds();
  virtual void GetActors(vtkPropCollection *pc);
  virtual void ReleaseGraphicsResources(vtkWindow*);
  virtual int RenderOverlay(vtkViewport*);
  virtual int HasTranslucentPolygonalGeometry();

protected:
  vtkTexturedButtonRepresentation2D();
  ~vtkTexturedButtonRepresentation2D();

  // Representing the button
  vtkBalloonRepresentation *Balloon;

  // Properties of the button
  vtkProperty2D *Property;
  vtkProperty2D *HoveringProperty;
  vtkProperty2D *SelectingProperty;
  void CreateDefaultProperties();

  // Keep track of the images (textures) associated with the N
  // states of the button.
  vtkTextureArray *TextureArray;

  // Tracking world position
  vtkCoordinate *Anchor;

private:
  vtkTexturedButtonRepresentation2D(const vtkTexturedButtonRepresentation2D&);  //Not implemented
  void operator=(const vtkTexturedButtonRepresentation2D&);  //Not implemented
};

#endif
