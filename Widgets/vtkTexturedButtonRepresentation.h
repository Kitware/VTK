/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTexturedButtonRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTexturedButtonRepresentation - defines a representation for a vtkButtonWidget
// .SECTION Description
// This class implements one type of vtkButtonRepresentation. It changes the
// appearance of a user-provided polydata by assigning textures according to the
// current button state. It also provides highlighting (when hovering and
// selecting the button) by fiddling with the actor's property.
//
// To use this representation, always begin by specifying the number of
// button states.  Then provide a polydata (the polydata should have associated
// texture coordinates), and a list of textures cooresponding to the button
// states. Optionally, the HoveringProperty and SelectionProperty can be
// adjusted to obtain the appropriate appearance.
//
// This widget representation has two placement methods. The conventional
// PlaceWidget() method is used to locate the textured button inside of a
// user-specified bounding box (note that the button geometry is uniformly
// scaled to fit, thus two of the three dimensions can be "large" and the
// third used to perform the scaling). However this PlaceWidget() method will
// align the geometry within x-y-z oriented bounds. To further control the
// placement, use the additional PlaceWidget(scale,point,normal) method. This
// scales the geometry, places its center at the specified point position,
// and orients the geometry's z-direction parallel to the specified normal.
// This can be used to attach "sticky notes" or "sticky buttons" to the
// surface of objects.

// .SECTION See Also
// vtkButtonWidget vtkButtonRepresentation vtkButtonSource vtkEllipticalButtonSource
// vtkRectangularButtonSource


#ifndef __vtkTexturedButtonRepresentation_h
#define __vtkTexturedButtonRepresentation_h

#include "vtkButtonRepresentation.h"

class vtkCellPicker;
class vtkActor;
class vtkProperty;
class vtkImageData;
class vtkTextureArray; //PIMPLd
class vtkPolyData;
class vtkPolyDataMapper;
class vtkAlgorithmOutput;
class vtkTexture;
class vtkFollower;

class VTK_WIDGETS_EXPORT vtkTexturedButtonRepresentation : public vtkButtonRepresentation
{
public:
  // Description:
  // Instantiate the class.
  static vtkTexturedButtonRepresentation *New();

  // Description:
  // Standard methods for instances of the class.
  vtkTypeMacro(vtkTexturedButtonRepresentation,vtkButtonRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the polydata which defines the button geometry.
  void SetButtonGeometry(vtkPolyData *pd);
  void SetButtonGeometryConnection(vtkAlgorithmOutput* algOutput);
  vtkPolyData *GetButtonGeometry();

  // Description:
  // Specify whether the button should always face the camera. If enabled,
  // the button rotates as the camera moves.
  vtkSetMacro(FollowCamera,int);
  vtkGetMacro(FollowCamera,int);
  vtkBooleanMacro(FollowCamera,int);

  // Description:
  // Specify the property to use when the button is to appear "normal"
  // i.e., the mouse pointer is not hovering or selecting the button.
  virtual void SetProperty(vtkProperty *p);
  vtkGetObjectMacro(Property,vtkProperty);

  // Description:
  // Specify the property to use when the hovering over the button.
  virtual void SetHoveringProperty(vtkProperty *p);
  vtkGetObjectMacro(HoveringProperty,vtkProperty);

  // Description:
  // Specify the property to use when selecting the button.
  virtual void SetSelectingProperty(vtkProperty *p);
  vtkGetObjectMacro(SelectingProperty,vtkProperty);

  // Description:
  // Add the ith texture corresponding to the ith button state.
  // The parameter i should be (0 <= i < NumberOfStates).
  void SetButtonTexture(int i, vtkImageData *image);
  vtkImageData *GetButtonTexture(int i);

  // Description:
  // Alternative method for placing a button at a given position (defined by
  // point[3]); at a given orientation (normal[3], where the z-axis of the
  // button geometry is parallel to the normal); and scaled by the scale
  // parameter. This method can bs used to attach "sticky notes" or "sticky
  // buttons" to objects. A great way to attach interactive meta-data to 3D
  // actors.
  virtual void PlaceWidget(double scale, double point[3], double normal[3]);

  // Description:
  // Provide the necessary methods to satisfy the vtkWidgetRepresentation API.
  virtual int ComputeInteractionState(int X, int Y, int modify=0);
  virtual void PlaceWidget(double bounds[6]);
  virtual void BuildRepresentation();
  virtual void Highlight(int state);

  // Description:
  // Provide the necessary methods to satisfy the rendering API.
  virtual void ShallowCopy(vtkProp *prop);
  virtual double *GetBounds();
  virtual void GetActors(vtkPropCollection *pc);
  virtual void ReleaseGraphicsResources(vtkWindow*);
  virtual int RenderOpaqueGeometry(vtkViewport*);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport*);
  virtual int HasTranslucentPolygonalGeometry();

protected:
  vtkTexturedButtonRepresentation();
  ~vtkTexturedButtonRepresentation();

  // Representing the button
  vtkActor          *Actor;
  vtkFollower       *Follower;
  vtkPolyDataMapper *Mapper;
  vtkTexture        *Texture;

  // Camera
  int FollowCamera;

  // Properties of the button
  vtkProperty *Property;
  vtkProperty *HoveringProperty;
  vtkProperty *SelectingProperty;
  void CreateDefaultProperties();

  // Keep track of the images (textures) associated with the N
  // states of the button. This is a PIMPLd stl map.
  vtkTextureArray *TextureArray;

  // For picking the button
  vtkCellPicker *Picker;

private:
  vtkTexturedButtonRepresentation(const vtkTexturedButtonRepresentation&);  //Not implemented
  void operator=(const vtkTexturedButtonRepresentation&);  //Not implemented
};

#endif
