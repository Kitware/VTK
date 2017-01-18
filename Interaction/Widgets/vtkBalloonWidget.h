/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBalloonWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBalloonWidget
 * @brief   popup text balloons above instance of vtkProp when hovering occurs
 *
 * The vtkBalloonWidget is used to popup text and/or an image when the mouse
 * hovers over an instance of vtkProp. The widget keeps track of
 * (vtkProp,vtkBalloon) pairs (where the internal vtkBalloon class is defined
 * by a pair of vtkStdString and vtkImageData), and when the mouse stops
 * moving for a user-specified period of time over the vtkProp, then the
 * vtkBalloon is drawn nearby the vtkProp. Note that an instance of
 * vtkBalloonRepresentation is used to draw the balloon.
 *
 * To use this widget, specify an instance of vtkBalloonWidget and a
 * representation (e.g., vtkBalloonRepresentation). Then list all instances
 * of vtkProp, a text string, and/or an instance of vtkImageData to be
 * associated with each vtkProp. (Note that you can specify both text and an
 * image, or just one or the other.) You may also wish to specify the hover
 * delay (i.e., set in the superclass vtkHoverWidget).
 *
 * @par Event Bindings:
 * By default, the widget observes the following VTK events (i.e., it
 * watches the vtkRenderWindowInteractor for these events):
 * <pre>
 *   MouseMoveEvent - occurs when mouse is moved in render window.
 *   TimerEvent - occurs when the time between events (e.g., mouse move)
 *                is greater than TimerDuration.
 *   KeyPressEvent - when the "Enter" key is pressed after the balloon appears,
 *                   a callback is activated (e.g., WidgetActivateEvent).
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's vtkWidgetEventTranslator. This class translates VTK events
 * into the vtkBalloonWidget's widget events:
 * <pre>
 *   vtkWidgetEvent::Move -- start the timer
 *   vtkWidgetEvent::TimedOut -- when hovering occurs,
 *   vtkWidgetEvent::SelectAction -- activate any callbacks associated
 *                                   with the balloon.
 * </pre>
 *
 * @par Event Bindings:
 * This widget invokes the following VTK events on itself (which observers
 * can listen for):
 * <pre>
 *   vtkCommand::TimerEvent (when hovering is determined to occur)
 *   vtkCommand::EndInteractionEvent (after a hover has occurred and the
 *                                    mouse begins moving again).
 *   vtkCommand::WidgetActivateEvent (when the balloon is selected with a
 *                                    keypress).
 * </pre>
 *
 * @sa
 * vtkAbstractWidget
*/

#ifndef vtkBalloonWidget_h
#define vtkBalloonWidget_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkHoverWidget.h"

class vtkBalloonRepresentation;
class vtkProp;
class vtkAbstractPropPicker;
class vtkStdString;
class vtkPropMap;
class vtkImageData;


class VTKINTERACTIONWIDGETS_EXPORT vtkBalloonWidget : public vtkHoverWidget
{
public:
  /**
   * Instantiate this class.
   */
  static vtkBalloonWidget *New();

  //@{
  /**
   * Standard methods for a VTK class.
   */
  vtkTypeMacro(vtkBalloonWidget,vtkHoverWidget);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  /**
   * The method for activating and deactivating this widget. This method
   * must be overridden because it performs special timer-related operations.
   */
  void SetEnabled(int) VTK_OVERRIDE;

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkBalloonRepresentation *r)
    {this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));}

  /**
   * Return the representation as a vtkBalloonRepresentation.
   */
  vtkBalloonRepresentation *GetBalloonRepresentation()
    {return reinterpret_cast<vtkBalloonRepresentation*>(this->WidgetRep);}

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() VTK_OVERRIDE;

  //@{
  /**
   * Add and remove text and/or an image to be associated with a vtkProp. You
   * may add one or both of them.
   */
  void AddBalloon(vtkProp *prop, vtkStdString *str, vtkImageData *img);
  void AddBalloon(vtkProp *prop, const char *str, vtkImageData *img);
  void AddBalloon(vtkProp *prop, const char *str) //for wrapping
    {this->AddBalloon(prop,str,NULL);}
  void RemoveBalloon(vtkProp *prop);
  //@}

  //@{
  /**
   * Methods to retrieve the information associated with each vtkProp (i.e.,
   * the information that makes up each balloon). A NULL will be returned if
   * the vtkProp does not exist, or if a string or image have not been
   * associated with the specified vtkProp.
   */
  const char *GetBalloonString(vtkProp *prop);
  vtkImageData *GetBalloonImage(vtkProp *prop);
  //@}

  //@{
  /**
   * Update the balloon string or image. If the specified prop does not exist,
   * then nothing is added not changed.
   */
  void UpdateBalloonString(vtkProp *prop, const char *str);
  void UpdateBalloonImage(vtkProp *prop, vtkImageData *image);
  //@}

  /**
   * Return the current vtkProp that is being hovered over. Note that the
   * value may be NULL (if hovering over nothing or the mouse is moving).
   */
  virtual vtkProp *GetCurrentProp()
    {return this->CurrentProp;}

  //@{
  /**
   * Set/Get the object used to perform pick operations. Since the
   * vtkBalloonWidget operates on vtkProps, the picker must be a subclass of
   * vtkAbstractPropPicker. (Note: if not specified, an instance of
   * vtkPropPicker is used.)
   */
  void SetPicker(vtkAbstractPropPicker*);
  vtkGetObjectMacro(Picker,vtkAbstractPropPicker);
  //@}

protected:
  vtkBalloonWidget();
  ~vtkBalloonWidget() VTK_OVERRIDE;

  // This class implements the method called from its superclass.
  int SubclassEndHoverAction() VTK_OVERRIDE;
  int SubclassHoverAction() VTK_OVERRIDE;

  // Classes for managing balloons
  vtkPropMap *PropMap; //PIMPL'd map of (vtkProp,vtkStdString)

  // Support for picking
  vtkAbstractPropPicker *Picker;

  // Register internal Pickers within PickingManager
  void RegisterPickers() VTK_OVERRIDE;

  // The vtkProp that is being hovered over (which may be NULL)
  vtkProp *CurrentProp;

private:
  vtkBalloonWidget(const vtkBalloonWidget&) VTK_DELETE_FUNCTION;
  void operator=(const vtkBalloonWidget&) VTK_DELETE_FUNCTION;
};

#endif
