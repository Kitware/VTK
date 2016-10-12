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

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWidgetRepresentation.h"

class VTKINTERACTIONWIDGETS_EXPORT vtkContinuousValueWidgetRepresentation :
  public vtkWidgetRepresentation
{
public:
  //@{
  /**
   * Standard methods for the class.
   */
  vtkTypeMacro(vtkContinuousValueWidgetRepresentation,
                       vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);
  //@}

  //@{
  /**
   * Methods to interface with the vtkSliderWidget. The PlaceWidget() method
   * assumes that the parameter bounds[6] specifies the location in display
   * space where the widget should be placed.
   */
  virtual void PlaceWidget(double bounds[6]);
  virtual void BuildRepresentation() {}
  virtual void StartWidgetInteraction(double eventPos[2]) = 0;
  virtual void WidgetInteraction(double eventPos[2]) = 0;
//  virtual void Highlight(int);
  //@}

  // Enums are used to describe what is selected
  enum _InteractionState
  {
    Outside=0,
    Inside,
    Adjusting
  };

  // Set/Get the value
  virtual void SetValue(double value);
  virtual double GetValue() {return this->Value;};

protected:
  vtkContinuousValueWidgetRepresentation();
  ~vtkContinuousValueWidgetRepresentation();

  double Value;

private:
  vtkContinuousValueWidgetRepresentation
  (const vtkContinuousValueWidgetRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkContinuousValueWidgetRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
