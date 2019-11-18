/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRPropPicker.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenVRPropPicker
 * @brief   Deprecated. Use vtkPropPicker directly
 */

#ifndef vtkOpenVRPropPicker_h
#define vtkOpenVRPropPicker_h

#include "vtkPropPicker.h"
#include "vtkRenderingOpenVRModule.h" // For export macro

class vtkProp;

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRPropPicker : public vtkPropPicker
{
public:
  static vtkOpenVRPropPicker* New();

  vtkTypeMacro(vtkOpenVRPropPicker, vtkPropPicker);

  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkOpenVRPropPicker();
  ~vtkOpenVRPropPicker() override;

  void Initialize() override;

private:
  vtkOpenVRPropPicker(const vtkOpenVRPropPicker&) = delete; // Not implemented.
  void operator=(const vtkOpenVRPropPicker&) = delete;      // Not implemented.
};

#endif
