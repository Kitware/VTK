/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContinuousValueWidgetRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

/**
 * @class   vtkContinuousValueWidgetRepresentation
 * @brief   provide the representation for a continuous value
 *
 * This class is used mainly as a superclass for continuous value widgets
 *
 */

#ifndef vtkContinuousValueWidgetRepresentation_h
#define vtkContinuousValueWidgetRepresentation_h

#include "vtkDeprecation.h"              // For VTK_DEPRECATED_IN_9_2_0
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkLegacy.h"                   // for VTK_LEGACY_REMOVE
#include "vtkWidgetRepresentation.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKINTERACTIONWIDGETS_EXPORT vtkContinuousValueWidgetRepresentation
  : public vtkWidgetRepresentation
{
public:
  ///@{
  /**
   * Standard methods for the class.
   */
  vtkTypeMacro(vtkContinuousValueWidgetRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Methods to interface with the vtkSliderWidget. The PlaceWidget() method
   * assumes that the parameter bounds[6] specifies the location in display
   * space where the widget should be placed.
   */
  void PlaceWidget(double bounds[6]) override;
  void BuildRepresentation() override {}
  void StartWidgetInteraction(double eventPos[2]) override = 0;
  void WidgetInteraction(double eventPos[2]) override = 0;
  //  virtual void Highlight(int);
  ///@}

  // Enums are used to describe what is selected
  enum InteractionStateType
  {
    Outside = 0,
    Inside,
    Adjusting
  };
#if !defined(VTK_LEGACY_REMOVE)
  VTK_DEPRECATED_IN_9_2_0("because leading underscore is reserved")
  typedef InteractionStateType _InteractionState;
#endif

  // Set/Get the value
  virtual void SetValue(double value);
  virtual double GetValue() { return this->Value; }

protected:
  vtkContinuousValueWidgetRepresentation();
  ~vtkContinuousValueWidgetRepresentation() override;

  double Value;

private:
  vtkContinuousValueWidgetRepresentation(const vtkContinuousValueWidgetRepresentation&) = delete;
  void operator=(const vtkContinuousValueWidgetRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
