// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkAssignCoordinatesLayoutStrategy
 * @brief   uses array values to set vertex locations
 *
 *
 * Uses vtkAssignCoordinates to use values from arrays as the x, y, and z coordinates.
 */

#ifndef vtkAssignCoordinatesLayoutStrategy_h
#define vtkAssignCoordinatesLayoutStrategy_h

#include "vtkGraphLayoutStrategy.h"
#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkSmartPointer.h"        // For SP ivars

VTK_ABI_NAMESPACE_BEGIN
class vtkAssignCoordinates;

class VTKINFOVISLAYOUT_EXPORT vtkAssignCoordinatesLayoutStrategy : public vtkGraphLayoutStrategy
{
public:
  static vtkAssignCoordinatesLayoutStrategy* New();
  vtkTypeMacro(vtkAssignCoordinatesLayoutStrategy, vtkGraphLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * The array to use for the x coordinate values.
   */
  virtual void SetXCoordArrayName(const char* name);
  virtual const char* GetXCoordArrayName();
  ///@}

  ///@{
  /**
   * The array to use for the y coordinate values.
   */
  virtual void SetYCoordArrayName(const char* name);
  virtual const char* GetYCoordArrayName();
  ///@}

  ///@{
  /**
   * The array to use for the z coordinate values.
   */
  virtual void SetZCoordArrayName(const char* name);
  virtual const char* GetZCoordArrayName();
  ///@}

  /**
   * Perform the random layout.
   */
  void Layout() override;

protected:
  vtkAssignCoordinatesLayoutStrategy();
  ~vtkAssignCoordinatesLayoutStrategy() override;

  vtkSmartPointer<vtkAssignCoordinates> AssignCoordinates;

private:
  vtkAssignCoordinatesLayoutStrategy(const vtkAssignCoordinatesLayoutStrategy&) = delete;
  void operator=(const vtkAssignCoordinatesLayoutStrategy&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
