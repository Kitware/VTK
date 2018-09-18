/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearWipeRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkRectilinearWipeRepresentation
 * @brief   represent a vtkRectilinearWipeWidget
 *
 * This class is used to represent and render a vtkRectilinearWipeWidget. To
 * use this class, you need to specify an instance of a
 * vtkImageRectilinearWipe and vtkImageActor. This provides the information
 * for this representation to construct and place itself.
 *
 * The class may be subclassed so that alternative representations can
 * be created.  The class defines an API and a default implementation that
 * the vtkRectilinearWipeWidget interacts with to render itself in the scene.
 *
 * @warning
 * The separation of the widget event handling and representation enables
 * users and developers to create new appearances for the widget. It also
 * facilitates parallel processing, where the client application handles
 * events, and remote representations of the widget are slaves to the
 * client (and do not handle events).
 *
 * @sa
 * vtkRectilinearWipeWidget vtkWidgetRepresentation vtkAbstractWidget
*/

#ifndef vtkRectilinearWipeRepresentation_h
#define vtkRectilinearWipeRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWidgetRepresentation.h"

class vtkImageRectilinearWipe;
class vtkImageActor;
class vtkPoints;
class vtkCellArray;
class vtkPolyData;
class vtkProperty2D;
class vtkPolyDataMapper2D;
class vtkActor2D;


class VTKINTERACTIONWIDGETS_EXPORT vtkRectilinearWipeRepresentation : public vtkWidgetRepresentation
{
public:
  /**
   * Instantiate this class.
   */
  static vtkRectilinearWipeRepresentation *New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkRectilinearWipeRepresentation,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify an instance of vtkImageRectilinearWipe to manipulate.
   */
  void SetRectilinearWipe(vtkImageRectilinearWipe *wipe);
  vtkGetObjectMacro(RectilinearWipe,vtkImageRectilinearWipe);
  //@}

  //@{
  /**
   * Specify an instance of vtkImageActor to decorate.
   */
  void SetImageActor(vtkImageActor *imageActor);
  vtkGetObjectMacro(ImageActor,vtkImageActor);
  //@}

  //@{
  /**
   * The tolerance representing the distance to the widget (in pixels)
   * in which the cursor is considered to be on the widget, or on a
   * widget feature (e.g., a corner point or edge).
   */
  vtkSetClampMacro(Tolerance,int,1,10);
  vtkGetMacro(Tolerance,int);
  //@}

  //@{
  /**
   * Get the properties for the widget. This can be manipulated to set
   * different colors, line widths, etc.
   */
  vtkGetObjectMacro(Property,vtkProperty2D);
  //@}

  //@{
  /**
   * Subclasses of vtkRectilinearWipeRepresentation must implement these methods. These
   * are the methods that the widget and its representation use to
   * communicate with each other.
   */
  void BuildRepresentation() override;
  void StartWidgetInteraction(double eventPos[2]) override;
  void WidgetInteraction(double eventPos[2]) override;
  int ComputeInteractionState(int X, int Y, int modify=0) override;
  //@}

  // Enums define the state of the prop relative to the mouse pointer
  // position. Used by ComputeInteractionState() to communicate with the
  // widget.
  enum _InteractionState
  {
    Outside=0,
    MovingHPane,
    MovingVPane,
    MovingCenter
  };

  //@{
  /**
   * Methods to make this class behave as a vtkProp.
   */
  void GetActors2D(vtkPropCollection *) override;
  void ReleaseGraphicsResources(vtkWindow *) override;
  int RenderOverlay(vtkViewport *viewport) override;
  int RenderOpaqueGeometry(vtkViewport *viewport) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport *viewport) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

protected:
  vtkRectilinearWipeRepresentation();
  ~vtkRectilinearWipeRepresentation() override;

  // Instances that this class manipulates
  vtkImageRectilinearWipe *RectilinearWipe;
  vtkImageActor           *ImageActor;

  // The pick tolerance of the widget in pixels
  int Tolerance;

  // This is used to track the beginning of interaction with the prop
  double StartWipePosition[2];

  // Indicates which part of widget is currently active based on the
  // state of the instance of the vtkImageRectilinearWipe.
  int  ActiveParts;

  // Geometric structure of widget
  vtkPoints           *Points; // The nine points defining the widget geometry
  vtkCellArray        *Lines;  // lines defining the boundary
  vtkPolyData         *Wipe;
  vtkPolyDataMapper2D *WipeMapper;
  vtkActor2D          *WipeActor;
  vtkProperty2D       *Property;

  // These are used to track the coordinates (in display coordinate system)
  // of the mid-edge and center point of the widget
  double DP4[3];
  double DP5[3];
  double DP6[3];
  double DP7[3];
  double DP8[3];

  int Dims[3]; // Dimensions of the input image to the wipe
  int I; //the i-j define the plane that is being displayed
  int J;

private:
  vtkRectilinearWipeRepresentation(const vtkRectilinearWipeRepresentation&) = delete;
  void operator=(const vtkRectilinearWipeRepresentation&) = delete;
};

#endif
