/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAvatar.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkAvatar
 * @brief Renders head and hands for a user in VR
 *
 * Set position and orientation for the head and two hands,
 * shows an observer where the avatar is looking and pointing.
 */

#ifndef vtkAvatar_h
#define vtkAvatar_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkActor.h"

class VTKRENDERINGCORE_EXPORT vtkAvatar: public vtkActor
{
public:
  static vtkAvatar* New();
  vtkTypeMacro(vtkAvatar, vtkActor)
  void PrintSelf(ostream &os, vtkIndent indent) override;

  /**
   * Set/Get the head and hand transforms.
   */
  vtkGetVector3Macro(HeadPosition, double);
  vtkSetVector3Macro(HeadPosition, double);
  vtkGetVector3Macro(HeadOrientation, double);
  vtkSetVector3Macro(HeadOrientation, double);

  vtkGetVector3Macro(LeftHandPosition, double);
  vtkSetVector3Macro(LeftHandPosition, double);
  vtkGetVector3Macro(LeftHandOrientation, double);
  vtkSetVector3Macro(LeftHandOrientation, double);

  vtkGetVector3Macro(RightHandPosition, double);
  vtkSetVector3Macro(RightHandPosition, double);
  vtkGetVector3Macro(RightHandOrientation, double);
  vtkSetVector3Macro(RightHandOrientation, double);

protected:
  vtkAvatar();
  ~vtkAvatar() override;

  double HeadPosition[3];
  double HeadOrientation[3];
  double LeftHandPosition[3];
  double LeftHandOrientation[3];
  double RightHandPosition[3];
  double RightHandOrientation[3];

private:
  vtkAvatar(const vtkAvatar&) = delete;
  void operator=(const vtkAvatar&) = delete;
};

#endif // vtkAvatar_h
