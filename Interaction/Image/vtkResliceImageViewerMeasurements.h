/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkResliceImageViewerMeasurements.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkResliceImageViewerMeasurements
 * @brief   Manage measurements on a resliced image
 *
 * This class manages measurements on the resliced image. It toggles the
 * the visibility of the measurements based on whether the resliced image
 * is the same orientation as when the measurement was initially placed.
 * @sa
 * vtkResliceCursor vtkResliceCursorWidget vtkResliceCursorRepresentation
*/

#ifndef vtkResliceImageViewerMeasurements_h
#define vtkResliceImageViewerMeasurements_h

#include "vtkInteractionImageModule.h" // For export macro
#include "vtkObject.h"

class vtkResliceImageViewer;
class vtkAbstractWidget;
class vtkCallbackCommand;
class vtkCollection;
class vtkDistanceWidget;
class vtkAngleWidget;
class vtkBiDimensionalWidget;
class vtkHandleRepresentation;
class vtkHandleWidget;
class vtkCaptionWidget;
class vtkContourWidget;
class vtkSeedWidget;

class VTKINTERACTIONIMAGE_EXPORT vtkResliceImageViewerMeasurements : public vtkObject
{
public:

  //@{
  /**
   * Standard VTK methods.
   */
  static vtkResliceImageViewerMeasurements *New();
  vtkTypeMacro(vtkResliceImageViewerMeasurements,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * Render the measurements.
   */
  virtual void Render();

  //@{
  /**
   * Add / remove a measurement widget
   */
  virtual void AddItem(vtkAbstractWidget *);
  virtual void RemoveItem(vtkAbstractWidget *);
  virtual void RemoveAllItems();
  //@}

  //@{
  /**
   * Methods to change whether the widget responds to interaction.
   * Set this to Off to disable interaction. On by default.
   * Subclasses must override SetProcessEvents() to make sure
   * that they pass on the flag to all component widgets.
   */
  vtkSetClampMacro(ProcessEvents, vtkTypeBool, 0, 1);
  vtkGetMacro(ProcessEvents, vtkTypeBool);
  vtkBooleanMacro(ProcessEvents, vtkTypeBool);
  //@}

  //@{
  /**
   * Tolerance for Point-in-Plane check
   */
  vtkSetMacro( Tolerance, double );
  vtkGetMacro( Tolerance, double );
  //@}

  //@{
  /**
   * Set the reslice image viewer. This is automatically done in the class
   * vtkResliceImageViewer
   */
  virtual void SetResliceImageViewer( vtkResliceImageViewer * );
  vtkGetObjectMacro( ResliceImageViewer, vtkResliceImageViewer );
  //@}

  /**
   * Update the measurements. This is automatically called when the reslice
   * cursor's axes are change.
   */
  virtual void Update();

protected:
  vtkResliceImageViewerMeasurements();
  ~vtkResliceImageViewerMeasurements() override;

  //@{
  /**
   * Check if a measurement widget is on the resliced plane.
   */
  bool IsItemOnReslicedPlane( vtkAbstractWidget * w );
  bool IsWidgetOnReslicedPlane( vtkDistanceWidget * w );
  bool IsWidgetOnReslicedPlane( vtkAngleWidget * w );
  bool IsWidgetOnReslicedPlane( vtkBiDimensionalWidget * w );
  bool IsWidgetOnReslicedPlane( vtkCaptionWidget * w );
  bool IsWidgetOnReslicedPlane( vtkContourWidget * w );
  bool IsWidgetOnReslicedPlane( vtkSeedWidget * w );
  bool IsWidgetOnReslicedPlane( vtkHandleWidget * w );
  bool IsPointOnReslicedPlane( vtkHandleRepresentation * h );
  bool IsPositionOnReslicedPlane( double p[3] );
  //@}

  // Handles the events; centralized here for all widgets.
  static void ProcessEventsHandler(vtkObject* object, unsigned long event,
                            void* clientdata, void* calldata);

  vtkResliceImageViewer * ResliceImageViewer;
  vtkCollection         * WidgetCollection;

  // Handle the visibility of the measurements.
  vtkCallbackCommand    * EventCallbackCommand; //

  // Flag indicating if we should handle events.
  // On by default.
  vtkTypeBool ProcessEvents;

  // Tolerance for Point-in-plane computation
  double Tolerance;

private:
  vtkResliceImageViewerMeasurements(const vtkResliceImageViewerMeasurements&) = delete;
  void operator=(const vtkResliceImageViewerMeasurements&) = delete;
};

#endif
