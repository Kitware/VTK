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
 * @class   vtkOpenVRDefaultOverlay
 * @brief   OpenVR overlay
 *
 * vtkOpenVRDefaultOverlay support for VR overlays
 */

#ifndef vtkOpenVRDefaultOverlay_h
#define vtkOpenVRDefaultOverlay_h

#include "vtkOpenVROverlay.h"
#include "vtkRenderingOpenVRModule.h" // For export macro

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRDefaultOverlay : public vtkOpenVROverlay
{
public:
  static vtkOpenVRDefaultOverlay* New();
  vtkTypeMacro(vtkOpenVRDefaultOverlay, vtkOpenVROverlay);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Render the overlay, we set some opf the spots based on current settings
   */
  void Render() override;

protected:
  vtkOpenVRDefaultOverlay();
  ~vtkOpenVRDefaultOverlay() override;

  void SetupSpots() override;

private:
  vtkOpenVRDefaultOverlay(const vtkOpenVRDefaultOverlay&) = delete;
  void operator=(const vtkOpenVRDefaultOverlay&) = delete;
};

#endif
