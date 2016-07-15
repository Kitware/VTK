/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkAbstractInteractionDevice_h
#define vtkAbstractInteractionDevice_h

#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro

class vtkRenderWidget;
class vtkAbstractRenderDevice;

class VTKRENDERINGCORE_EXPORT vtkAbstractInteractionDevice : public vtkObject
{
public:
  vtkTypeMacro(vtkAbstractInteractionDevice, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * @brief Make a new device, this class is abstract and one of its derived
   * forms will be returned, or NULL if no override has been provided.
   * @return A derived interaction device, or NULL if no suitable override is set.
   */
  static vtkAbstractInteractionDevice* New();

  /**
   * @brief Initialize the interaction device.
   */
  virtual void Initialize() = 0;

  /**
   * @brief Start the event loop.
   */
  virtual void Start() = 0;

  /**
   * @brief Process any pending events, this can be used to process OS level
   * events without running a full event loop.
   */
  virtual void ProcessEvents() = 0;

  void SetRenderWidget(vtkRenderWidget *widget);
  vtkRenderWidget* GetRenderWidget() { return this->RenderWidget; }
  void SetRenderDevice(vtkAbstractRenderDevice *device);
  vtkAbstractRenderDevice* GetRenderDevice() { return this->RenderDevice; }

protected:
  vtkAbstractInteractionDevice();
  ~vtkAbstractInteractionDevice();

  bool Initialized;
  vtkRenderWidget *RenderWidget;
  vtkAbstractRenderDevice *RenderDevice;

private:
  vtkAbstractInteractionDevice(const vtkAbstractInteractionDevice&) VTK_DELETE_FUNCTION;
  void operator=(const vtkAbstractInteractionDevice&) VTK_DELETE_FUNCTION;
};

#endif
