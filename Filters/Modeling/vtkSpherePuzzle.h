// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSpherePuzzle
 * @brief   create a polygonal sphere centered at the origin
 *
 * vtkSpherePuzzle creates
 */

#ifndef vtkSpherePuzzle_h
#define vtkSpherePuzzle_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#define VTK_MAX_SPHERE_RESOLUTION 1024

VTK_ABI_NAMESPACE_BEGIN
class vtkTransform;

class VTKFILTERSMODELING_EXPORT vtkSpherePuzzle : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkSpherePuzzle, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkSpherePuzzle* New();

  /**
   * Reset the state of this puzzle back to its original state.
   */
  void Reset();

  /**
   * Move the top/bottom half one segment either direction.
   */
  void MoveHorizontal(int section, int percentage, int rightFlag);

  /**
   * Rotate vertical half of sphere along one of the longitude lines.
   */
  void MoveVertical(int section, int percentage, int rightFlag);

  /**
   * SetPoint will be called as the mouse moves over the screen.
   * The output will change to indicate the pending move.
   * SetPoint returns zero if move is not activated by point.
   * Otherwise it encodes the move into a unique integer so that
   * the caller can determine if the move state has changed.
   * This will answer the question, "Should I render."
   */
  int SetPoint(double x, double y, double z);

  /**
   * Move actually implements the pending move. When percentage
   * is 100, the pending move becomes inactive, and SetPoint
   * will have to be called again to setup another move.
   */
  void MovePoint(int percentage);

  /**
   * For drawing state as arrows.
   */
  int* GetState() { return this->State; }

protected:
  vtkSpherePuzzle();
  ~vtkSpherePuzzle() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  void MarkVertical(int section);
  void MarkHorizontal(int section);

  int State[32];

  // Stuff for storing a partial move.
  int PieceMask[32];
  vtkTransform* Transform;

  // Colors for faces.
  unsigned char Colors[96];

  // State for potential move.
  int Active;
  int VerticalFlag;
  int RightFlag;
  int Section;

private:
  vtkSpherePuzzle(const vtkSpherePuzzle&) = delete;
  void operator=(const vtkSpherePuzzle&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
