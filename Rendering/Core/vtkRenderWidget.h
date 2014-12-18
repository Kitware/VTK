/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkRenderWidget_h
#define vtkRenderWidget_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"
#include "vtkNew.h" // For member variables.
#include "vtkVector.h" // For member variables.
#include <string> // For member variables.

class vtkAbstractInteractionDevice;
class vtkAbstractRenderDevice;

class VTKRENDERINGCORE_EXPORT vtkRenderWidget : public vtkObject
{
public:
  vtkTypeMacro(vtkRenderWidget ,vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkRenderWidget* New();

  /**
   * @brief Set the widget position in screen coordinates.
   * @param pos The position of the widget in screen coordinates.
   */
  void SetPosition(const vtkVector2i &pos);

  /**
   * @brief Get the widget position in screen coordinates.
   * @return The position of the widget in screen coordinates, default of 0, 0.
   */
  vtkVector2i GetPosition() const { return this->Position; }

  /**
   * @brief Set the widget size in screen coordinates.
   * @param size The width and height of the widget in screen coordinates
   */
  void SetSize(const vtkVector2i &size);

  /**
   * @brief Get the widget size in screen coordinates.
   * @return The width and height of the widget in screen coordinates, default
   * of 300x300.
   */
  vtkVector2i GetSize() const { return this->Size; }

  /**
   * @brief Set the name of the widget.
   * @param name The name to set to the window.
   */
  void SetName(const std::string &name);

  /**
   * @brief Get the name of the widget.
   * @return The current name of the widget.
   */
  std::string GetName() const { return this->Name; }

  /**
   * @brief Render everything in the current widget.
   */
  virtual void Render();

  /**
   * @brief Make the widget's context current, this will defer to the OS
   * specific methods, and calls should be kept to a minimum as they are quite
   * expensive.
   */
  virtual void MakeCurrent();

  void Initialize();
  void Start();

protected:
  vtkRenderWidget();
  ~vtkRenderWidget();

  vtkVector2i Position; // Position of the widget in screen coordinates.
  vtkVector2i Size; // Position of the widget in screen coordinates.
  std::string Name; // The name of the widget.

  vtkNew<vtkAbstractInteractionDevice> InteractionDevice; // Interaction device.
  vtkNew<vtkAbstractRenderDevice> RenderDevice; // Render device target.

private:
  vtkRenderWidget(const vtkRenderWidget&);  // Not implemented.
  void operator=(const vtkRenderWidget&);  // Not implemented.
};

#endif
