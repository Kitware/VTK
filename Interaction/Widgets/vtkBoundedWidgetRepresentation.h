// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBoundedWidgetRepresentation
 * @brief   Extends the vtkWidgetRepresentation to help positioning widget in space
  and how it should be displayed regarding to input bounds.
 *
 * This adds an Outline feature for the widget: an interactive box can be displayed,
 * representing the bounding box of the widget. This is usually initialized
 * with some input data bounding box (for instance when using widget to create a slice).
 *
 * Then, different flags control the behavior of the widget origin interactions:
 * - OutsideBounds: when on (default), the origin of the widget can move outside the
 * InitialBounds (see vtkWidgetRepresentation::PlaceWidget)
 * - ConstrainToWidgetBounds: when on (default), the origin of the Widget cannot move outside
 * the WidgetBounds. When off, the Outline is extended as needed to contains the origin.
 */

#ifndef vtkBoundedWidgetRepresentation_h
#define vtkBoundedWidgetRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkNew.h"
#include "vtkWidgetRepresentation.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN

class vtkActor;
class vtkImageData;
class vtkOutlineFilter;
class vtkPolyDataMapper;
class vtkProperty;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkBoundedWidgetRepresentation
  : public vtkWidgetRepresentation
{
public:
  ///@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkBoundedWidgetRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Turn On/Off the ability to translate the bounding box
   * from mouse interaction.
   * Default is On.
   * @see GetTranslationAxis
   */
  ///@{
  vtkSetMacro(OutlineTranslation, bool);
  vtkGetMacro(OutlineTranslation, bool);
  vtkBooleanMacro(OutlineTranslation, bool);
  ///@}

  /**
   * Turn On/Off the ability to move the widget origin outside the bounds
   * defined by the PlaceWidget call.
   * Default is On.
   */
  ///@{
  vtkSetMacro(OutsideBounds, bool);
  vtkGetMacro(OutsideBounds, bool);
  vtkBooleanMacro(OutsideBounds, bool);
  ///@}

  /**
   * Turn On/Off whether the widget origin should be contained in WidgetBounds.
   * When Off, the Outline is extended as needed to contain the origin.
   * Default is On.
   */
  ///@{
  vtkSetMacro(ConstrainToWidgetBounds, bool);
  vtkGetMacro(ConstrainToWidgetBounds, bool);
  vtkBooleanMacro(ConstrainToWidgetBounds, bool);
  ///@}

  /**
   * Get the outline property. The Selected version is used
   * the indicate interaction on the outline.
   * @see HighlightOutline()
   */
  ///@{
  vtkGetObjectMacro(OutlineProperty, vtkProperty);
  vtkGetObjectMacro(SelectedOutlineProperty, vtkProperty);
  ///@}

  ///@{
  /**
   * Set/Get the bounds of the widget representation. PlaceWidget can also be
   * used to set the bounds of the widget but it may also have other effects
   * on the internal state of the representation. Use this function when only
   * the widget bounds need to be modified.
   */
  vtkSetVector6Macro(WidgetBounds, double);
  vtkGetVector6Macro(WidgetBounds, double);
  ///@}

  ///@{
  /**
   * Gets/Sets the constraint axis for translations. Returns Axis::NONE
   * if none.
   * Default is Axis::NONE (-1)
   * @see GetOutlineTranslation()
   **/
  vtkGetMacro(TranslationAxis, int);
  vtkSetClampMacro(TranslationAxis, int, -1, 2);
  ///@}

  ///@{
  /**
   * Constrains translation on given axis.
   * @see SetTranslationAxis()
   */
  void SetXTranslationAxisOn() { this->SetTranslationAxis(Axis::XAxis); }
  void SetYTranslationAxisOn() { this->SetTranslationAxis(Axis::YAxis); }
  void SetZTranslationAxisOn() { this->SetTranslationAxis(Axis::ZAxis); }
  void SetTranslationAxisOff() { this->SetTranslationAxis(Axis::NONE); }
  ///@}

protected:
  vtkBoundedWidgetRepresentation();
  ~vtkBoundedWidgetRepresentation() override;

  vtkActor* GetOutlineActor() { return this->OutlineActor; }

  /**
   * Switch between outline properties depending on highlight.
   * When highlight is On, use SelectedOutlineProperty.
   * @see GetOutlineProperty, GetSelectedOutlineProperty
   */
  void HighlightOutline(int highlight);

  /**
   * Translate outline from point p1 to point p2.
   * Internally call TranslateRepresentation.
   * @see TranslateRepresentation
   */
  void TranslateOutline(double* p1, double* p2);

  /**
   * Translate the representation, to be implemented in derived class.
   * No-op by default.
   * @see TranslateOutline
   */
  virtual void TranslateRepresentation(const vtkVector3d&){};

  /**
   * Returns true if Axis is constrained, i.e. if TranslationAxis is set to
   * any other value than NONE.
   * @see vtkWidgetRepresentation::Axis, SetTranslationAxis()
   **/
  bool IsTranslationConstrained() { return this->TranslationAxis != Axis::NONE; }

  /**
   * Return the Outline diagonal length.
   */
  double GetDiagonalLength();

  /**
   * Set/Get the outline bounds.
   */
  ///@{
  void SetOutlineBounds(double bounds[6]);
  void GetOutlineBounds(double bounds[6]);
  ///@}

  /**
   * Transform the current outline bounds using given transform.
   * Also update WidgetBounds accordingly.
   */
  void TransformBounds(vtkTransform* transform);

  /**
   * Modify Center argument to clamp it into bounds, if required.
   * Update outline bounds accordingly.
   * @see GetOutsideBounds(), GetConstrainToWidgetBounds()
   */
  void UpdateCenterAndBounds(double center[6]);

  /**
   * Create and initialize properties with default values.
   */
  virtual void CreateDefaultProperties();

  /**
   * Set the default colors for outline.
   */
  ///@{
  void SetOutlineColor(double r, double g, double b);
  void SetSelectedOutlineColor(double r, double g, double b);
  ///@}

  /**
   * Ensure outline is up to date.
   */
  void UpdateOutline();

  vtkImageData* GetBox() { return this->Box; }

private:
  vtkBoundedWidgetRepresentation(const vtkBoundedWidgetRepresentation&) = delete;
  void operator=(const vtkBoundedWidgetRepresentation&) = delete;

  /**
   * Get and store widget current bounds.
   */
  void UpdateWidgetBounds();

  vtkNew<vtkImageData> Box;
  vtkNew<vtkOutlineFilter> Outline;
  vtkNew<vtkPolyDataMapper> OutlineMapper;
  vtkNew<vtkActor> OutlineActor;

  vtkNew<vtkProperty> OutlineProperty;
  vtkNew<vtkProperty> SelectedOutlineProperty;

  bool OutlineTranslation = true;
  bool OutsideBounds = true;
  bool ConstrainToWidgetBounds = true;

  int TranslationAxis = Axis::NONE;

  double WidgetBounds[6] = { 1, -1, 1, -1, 1, -1 };
};

VTK_ABI_NAMESPACE_END
#endif
