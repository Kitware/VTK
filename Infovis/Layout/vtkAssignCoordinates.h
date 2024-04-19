// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkAssignCoordinates
 * @brief   Given two(or three) arrays take the values
 * in those arrays and simply assign them to the coordinates of the vertices.
 *
 *
 * Given two(or three) arrays take the values in those arrays and simply assign
 * them to the coordinates of the vertices. Yes you could do this with the array
 * calculator, but your mom wears army boots so we're not going to.
 */

#ifndef vtkAssignCoordinates_h
#define vtkAssignCoordinates_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKINFOVISLAYOUT_EXPORT vtkAssignCoordinates : public vtkPassInputTypeAlgorithm
{
public:
  static vtkAssignCoordinates* New();

  vtkTypeMacro(vtkAssignCoordinates, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the x coordinate array name.
   */
  vtkSetStringMacro(XCoordArrayName);
  vtkGetStringMacro(XCoordArrayName);
  ///@}

  ///@{
  /**
   * Set the y coordinate array name.
   */
  vtkSetStringMacro(YCoordArrayName);
  vtkGetStringMacro(YCoordArrayName);
  ///@}

  ///@{
  /**
   * Set the z coordinate array name.
   */
  vtkSetStringMacro(ZCoordArrayName);
  vtkGetStringMacro(ZCoordArrayName);
  ///@}

  ///@{
  /**
   * Set if you want a random jitter
   */
  vtkSetMacro(Jitter, bool);
  ///@}

protected:
  vtkAssignCoordinates();
  ~vtkAssignCoordinates() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  char* XCoordArrayName;
  char* YCoordArrayName;
  char* ZCoordArrayName;
  bool Jitter;

  vtkAssignCoordinates(const vtkAssignCoordinates&) = delete;
  void operator=(const vtkAssignCoordinates&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
