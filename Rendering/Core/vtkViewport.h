/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkViewport.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkViewport
 * @brief   abstract specification for Viewports
 *
 * vtkViewport provides an abstract specification for Viewports. A Viewport
 * is an object that controls the rendering process for objects. Rendering
 * is the process of converting geometry, a specification for lights, and
 * a camera view into an image. vtkViewport also performs coordinate
 * transformation between world coordinates, view coordinates (the computer
 * graphics rendering coordinate system), and display coordinates (the
 * actual screen coordinates on the display device). Certain advanced
 * rendering features such as two-sided lighting can also be controlled.
 *
 * @sa
 * vtkWindow vtkRenderer
*/

#ifndef vtkViewport_h
#define vtkViewport_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkActor2DCollection;
class vtkAssemblyPath;
class vtkProp;
class vtkPropCollection;
class vtkWindow;

class VTKRENDERINGCORE_EXPORT vtkViewport : public vtkObject
{
public:
  vtkTypeMacro(vtkViewport,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Add a prop to the list of props. Does nothing if the prop is
   * already present. Prop is the superclass of all actors, volumes,
   * 2D actors, composite props etc.
   */
  void AddViewProp(vtkProp *);

  /**
   * Return any props in this viewport.
   */
  vtkPropCollection *GetViewProps() {return this->Props;};

  /**
   * Query if a prop is in the list of props.
   */
  int HasViewProp(vtkProp *);

  /**
   * Remove a prop from the list of props. Does nothing if the prop
   * is not already present.
   */
  void RemoveViewProp(vtkProp *);

  /**
   * Remove all props from the list of props.
   */
  void RemoveAllViewProps(void);

  //@{
  /**
   * Add/Remove different types of props to the renderer.
   * These methods are all synonyms to AddViewProp and RemoveViewProp.
   * They are here for convenience and backwards compatibility.
   */
  void AddActor2D(vtkProp* p);
  void RemoveActor2D(vtkProp* p);
  vtkActor2DCollection *GetActors2D();
  //@}

  //@{
  /**
   * Set/Get the background color of the rendering screen using an rgb color
   * specification.
   */
  vtkSetVector3Macro(Background,double);
  vtkGetVector3Macro(Background,double);
  //@}

  //@{
  /**
   * Set/Get the second background color of the rendering screen
   * for gradient backgrounds using an rgb color specification.
   */
  vtkSetVector3Macro(Background2,double);
  vtkGetVector3Macro(Background2,double);
  //@}

  //@{
  /**
   * Set/Get whether this viewport should have a gradient background
   * using the Background (bottom) and Background2 (top) colors.
   * Default is off.
   */
  vtkSetMacro(GradientBackground,bool);
  vtkGetMacro(GradientBackground,bool);
  vtkBooleanMacro(GradientBackground,bool);
  //@}

  //@{
  /**
   * Set the aspect ratio of the rendered image. This is computed
   * automatically and should not be set by the user.
   */
  vtkSetVector2Macro(Aspect,double);
  vtkGetVectorMacro(Aspect,double,2);
  virtual void ComputeAspect();
  //@}

  //@{
  /**
   * Set the aspect ratio of a pixel in the rendered image.
   * This factor permits the image to rendered anisotropically
   * (i.e., stretched in one direction or the other).
   */
  vtkSetVector2Macro(PixelAspect,double);
  vtkGetVectorMacro(PixelAspect,double,2);
  //@}

  //@{
  /**
   * Specify the viewport for the Viewport to draw in the rendering window.
   * Coordinates are expressed as (xmin,ymin,xmax,ymax), where each
   * coordinate is 0 <= coordinate <= 1.0.
   */
  vtkSetVector4Macro(Viewport,double);
  vtkGetVectorMacro(Viewport,double,4);
  //@}

  //@{
  /**
   * Set/get a point location in display (or screen) coordinates.
   * The lower left corner of the window is the origin and y increases
   * as you go up the screen.
   */
  vtkSetVector3Macro(DisplayPoint,double);
  vtkGetVectorMacro(DisplayPoint,double,3);
  //@}

  //@{
  /**
   * Specify a point location in view coordinates. The origin is in the
   * middle of the viewport and it extends from -1 to 1 in all three
   * dimensions.
   */
  vtkSetVector3Macro(ViewPoint,double);
  vtkGetVectorMacro(ViewPoint,double,3);
  //@}

  //@{
  /**
   * Specify a point location in world coordinates. This method takes
   * homogeneous coordinates.
   */
  vtkSetVector4Macro(WorldPoint,double);
  vtkGetVectorMacro(WorldPoint,double,4);
  //@}

  /**
   * Return the center of this viewport in display coordinates.
   */
  virtual double *GetCenter();

  /**
   * Is a given display point in this Viewport's viewport.
   */
  virtual int IsInViewport(int x,int y);

  /**
   * Return the vtkWindow that owns this vtkViewport.
   */
  virtual vtkWindow *GetVTKWindow() = 0;

  /**
   * Convert display coordinates to view coordinates.
   */
  virtual void DisplayToView(); // these get modified in subclasses

  /**
   * Convert view coordinates to display coordinates.
   */
  virtual void ViewToDisplay(); // to handle stereo rendering

  /**
   * Convert world point coordinates to view coordinates.
   */
  virtual void WorldToView();

  /**
   * Convert view point coordinates to world coordinates.
   */
  virtual void ViewToWorld();

  /**
   * Convert display (or screen) coordinates to world coordinates.
   */
  void DisplayToWorld() {this->DisplayToView(); this->ViewToWorld();};

  /**
   * Convert world point coordinates to display (or screen) coordinates.
   */
  void WorldToDisplay() {this->WorldToView(); this->ViewToDisplay();};

  //@{
  /**
   * These methods map from one coordinate system to another.
   * They are primarily used by the vtkCoordinate object and
   * are often strung together. These methods return valid information
   * only if the window has been realized (e.g., GetSize() returns
   * something other than (0,0)).
   */
  virtual void LocalDisplayToDisplay(double &x, double &y);
  virtual void DisplayToNormalizedDisplay(double &u, double &v);
  virtual void NormalizedDisplayToViewport(double &x, double &y);
  virtual void ViewportToNormalizedViewport(double &u, double &v);
  virtual void NormalizedViewportToView(double &x, double &y, double &z);
  virtual void ViewToWorld(double &, double &, double &) {}
  virtual void DisplayToLocalDisplay(double &x, double &y);
  virtual void NormalizedDisplayToDisplay(double &u, double &v);
  virtual void ViewportToNormalizedDisplay(double &x, double &y);
  virtual void NormalizedViewportToViewport(double &u, double &v);
  virtual void ViewToNormalizedViewport(double &x, double &y, double &z);
  virtual void WorldToView(double &, double &, double &) {}
  //@}

  //@{
  /**
   * Get the size and origin of the viewport in display coordinates. Note:
   * if the window has not yet been realized, GetSize() and GetOrigin()
   * return (0,0).
   */
  virtual int *GetSize();
  virtual int *GetOrigin();
  void GetTiledSize(int *width, int *height);
  virtual void GetTiledSizeAndOrigin(int *width, int *height,
                                     int *lowerLeftX, int *lowerLeftY);
  //@}

  // The following methods describe the public pick interface for picking
  // Props in a viewport.

  /**
   * Return the Prop that has the highest z value at the given x, y position
   * in the viewport.  Basically, the top most prop that renders the pixel at
   * selectionX, selectionY will be returned.  If no Props are there NULL is
   * returned.  This method selects from the Viewports Prop list.
   */
  virtual vtkAssemblyPath* PickProp(double selectionX, double selectionY) = 0;

  /**
   * Return the Prop that has the highest z value at the given x1, y1
   * and x2,y2 positions in the viewport.  Basically, the top most prop that
   * renders the pixel at selectionX1, selectionY1, selectionX2, selectionY2
   * will be returned.  If no Props are there NULL is returned.  This method
   * selects from the Viewports Prop list.
   */
  virtual vtkAssemblyPath* PickProp(double selectionX1, double selectionY1,
                                    double selectionX2, double selectionY2) = 0;

  /**
   * Same as PickProp with two arguments, but selects from the given
   * collection of Props instead of the Renderers props.  Make sure
   * the Props in the collection are in this renderer.
   */
  vtkAssemblyPath* PickPropFrom(double selectionX, double selectionY,
                                vtkPropCollection*);

  /**
   * Same as PickProp with four arguments, but selects from the given
   * collection of Props instead of the Renderers props.  Make sure
   * the Props in the collection are in this renderer.
   */
  vtkAssemblyPath* PickPropFrom(double selectionX1, double selectionY1,
                                double selectionX2, double selectionY2,
                                vtkPropCollection*);

  //@{
  /**
   * Methods used to return the pick (x,y) in local display coordinates (i.e.,
   * it's that same as selectionX and selectionY).
   */
  double GetPickX() const {return (this->PickX1 + this->PickX2)*0.5;}
  double GetPickY() const {return (this->PickY1 + this->PickY2)*0.5;}
  double GetPickWidth() const {return this->PickX2 - this->PickX1 + 1;};
  double GetPickHeight() const {return this->PickY2 - this->PickY1 + 1;};
  double GetPickX1() const {return this->PickX1;}
  double GetPickY1() const {return this->PickY1;}
  double GetPickX2() const {return this->PickX2;}
  double GetPickY2() const {return this->PickY2;}
  vtkGetMacro(IsPicking, int);
  vtkGetMacro(CurrentPickId, unsigned int);
  void SetCurrentPickId(unsigned int a) {this->CurrentPickId = a;};
  vtkGetObjectMacro(PickResultProps, vtkPropCollection);
  //@}

  /**
   * Return the Z value for the last picked Prop.
   */
  virtual double GetPickedZ() = 0;

protected:
  // Create a vtkViewport with a black background, a white ambient light,
  // two-sided lighting turned on, a viewport of (0,0,1,1), and back face
  // culling turned off.
  vtkViewport();
  ~vtkViewport();

  // Picking functions to be implemented by sub-classes
  // Perform the main picking loop
  virtual void DevicePickRender() = 0;
  // Enter a pick mode
  virtual void StartPick(unsigned int pickFromSize) = 0;
  // Set the pick id to the next id before drawing an object
  virtual void UpdatePickId() = 0;
  // Exit Pick mode
  virtual void DonePick() = 0;
  // Return the id of the picked object, only valid after a call to DonePick
  virtual unsigned int GetPickedId() = 0;
  // Return the number of objects picked, only valid after a call to DonePick
  virtual unsigned int GetNumPickedIds() = 0;
  // Put no more than atMost picked object ids into the callerBuffer and
  // return the number of picked objects returned.
  virtual int GetPickedIds(unsigned int atMost, unsigned int *callerBuffer) = 0;

  // Ivars for picking
  // Store a picked Prop (contained in an assembly path)
  vtkAssemblyPath* PickedProp;
  vtkPropCollection* PickFromProps;
  vtkPropCollection* PickResultProps;
  // Boolean flag to determine if picking is enabled for this render
  int IsPicking;
  unsigned int CurrentPickId;
  double PickX1;
  double PickY1;
  double PickX2;
  double PickY2;
  // End Ivars for picking

  vtkPropCollection *Props;
  vtkActor2DCollection *Actors2D;
  vtkWindow *VTKWindow;
  double Background[3];
  double Background2[3];
  double Viewport[4];
  double Aspect[2];
  double PixelAspect[2];
  double Center[2];
  bool GradientBackground;

  int Size[2];
  int Origin[2];
  double DisplayPoint[3];
  double ViewPoint[3];
  double WorldPoint[4];


private:
  vtkViewport(const vtkViewport&) VTK_DELETE_FUNCTION;
  void operator=(const vtkViewport&) VTK_DELETE_FUNCTION;
};



#endif
