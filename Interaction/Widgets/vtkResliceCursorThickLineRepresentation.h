/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkResliceCursorLineRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkResliceCursorThickLineRepresentation
 * @brief   represents a thick slab of the reslice cursor widget
 *
 * This class respresents a thick reslice cursor, that can be used to
 * perform interactive thick slab MPR's through data. The class internally
 * uses vtkImageSlabReslice to do its reslicing. The slab thickness is set
 * interactively from the widget. The slab resolution (ie the number of
 * blend points) is set as the minimum spacing along any dimension from
 * the dataset.
 * @sa
 * vtkImageSlabReslice vtkResliceCursorLineRepresentation vtkResliceCursorWidget
*/

#ifndef vtkResliceCursorThickLineRepresentation_h
#define vtkResliceCursorThickLineRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkResliceCursorLineRepresentation.h"

class VTKINTERACTIONWIDGETS_EXPORT vtkResliceCursorThickLineRepresentation : public vtkResliceCursorLineRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static vtkResliceCursorThickLineRepresentation *New();

  //@{
  /**
   * Standard VTK methods.
   */
  vtkTypeMacro(vtkResliceCursorThickLineRepresentation,vtkResliceCursorLineRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);
  //@}

  /**
   * INTERNAL - Do not use
   * Create the thick reformat class. This overrides the superclass
   * implementation and creates a vtkImageSlabReslice instead of a
   * vtkImageReslice.
   */
  virtual void CreateDefaultResliceAlgorithm();

  /**
   * INTERNAL - Do not use
   * Reslice parameters which are set from vtkResliceCursorWidget based on
   * user interactions.
   */
  virtual void SetResliceParameters(
      double outputSpacingX, double outputSpacingY,
      int extentX, int extentY );

protected:
  vtkResliceCursorThickLineRepresentation();
  ~vtkResliceCursorThickLineRepresentation();

private:
  vtkResliceCursorThickLineRepresentation(const vtkResliceCursorThickLineRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkResliceCursorThickLineRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
