/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenVRFollower
 * @brief   OpenVR Follower
 *
 * vtkOpenVRFollower a follower that aligns with PhysicalViewUp
 */

#ifndef vtkOpenVRFollower_h
#define vtkOpenVRFollower_h

#include "vtkFollower.h"
#include "vtkRenderingOpenVRModule.h" // For export macro

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRFollower : public vtkFollower
{
public:
  static vtkOpenVRFollower* New();
  vtkTypeMacro(vtkOpenVRFollower, vtkFollower);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  virtual void Render(vtkRenderer* ren) override;

  /**
   * Generate the matrix based on ivars. This method overloads its superclasses
   * ComputeMatrix() method due to the special vtkFollower matrix operations.
   */
  void ComputeMatrix() override;

protected:
  vtkOpenVRFollower();
  ~vtkOpenVRFollower() override;

  double LastViewUp[3];

private:
  vtkOpenVRFollower(const vtkOpenVRFollower&) = delete;
  void operator=(const vtkOpenVRFollower&) = delete;
};

#endif
