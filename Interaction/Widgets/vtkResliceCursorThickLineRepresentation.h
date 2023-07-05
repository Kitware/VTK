// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkResliceCursorThickLineRepresentation
 * @brief   represents a thick slab of the reslice cursor widget
 *
 * This class represents a thick reslice cursor, that can be used to
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

VTK_ABI_NAMESPACE_BEGIN
class VTKINTERACTIONWIDGETS_EXPORT vtkResliceCursorThickLineRepresentation
  : public vtkResliceCursorLineRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static vtkResliceCursorThickLineRepresentation* New();

  ///@{
  /**
   * Standard VTK methods.
   */
  vtkTypeMacro(vtkResliceCursorThickLineRepresentation, vtkResliceCursorLineRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * INTERNAL - Do not use
   * Create the thick reformat class. This overrides the superclass
   * implementation and creates a vtkImageSlabReslice instead of a
   * vtkImageReslice.
   */
  void CreateDefaultResliceAlgorithm() override;

  /**
   * INTERNAL - Do not use
   * Reslice parameters which are set from vtkResliceCursorWidget based on
   * user interactions.
   */
  void SetResliceParameters(
    double outputSpacingX, double outputSpacingY, int extentX, int extentY) override;

protected:
  vtkResliceCursorThickLineRepresentation();
  ~vtkResliceCursorThickLineRepresentation() override;

private:
  vtkResliceCursorThickLineRepresentation(const vtkResliceCursorThickLineRepresentation&) = delete;
  void operator=(const vtkResliceCursorThickLineRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
