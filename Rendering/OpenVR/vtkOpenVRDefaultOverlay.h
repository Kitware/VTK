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

#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkOpenVROverlay.h"

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRDefaultOverlay : public vtkOpenVROverlay
{
public:
  static vtkOpenVRDefaultOverlay *New();
  vtkTypeMacro(vtkOpenVRDefaultOverlay, vtkOpenVROverlay);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Render the overlay, we set some opf the spots based on current settings
   */
  virtual void Render();

protected:
  vtkOpenVRDefaultOverlay();
  ~vtkOpenVRDefaultOverlay();

  virtual void SetupSpots();

private:
  vtkOpenVRDefaultOverlay(const vtkOpenVRDefaultOverlay&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenVRDefaultOverlay&) VTK_DELETE_FUNCTION;
};

#endif
