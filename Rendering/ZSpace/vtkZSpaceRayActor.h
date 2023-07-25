// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
* @class   vtkZSpaceRayActor
* @brief   An actor for displaying a ray.

* Represents a ray shooting from the ZSpace stylus, used for pointing or picking.
* Contains a line source. Use SetUserTransform to control the ray.
*/

#ifndef vtkZSpaceRayActor_h
#define vtkZSpaceRayActor_h

#include "vtkNew.h" // for ivar
#include "vtkOpenGLActor.h"
#include "vtkRenderingZSpaceModule.h" // for export macro

VTK_ABI_NAMESPACE_BEGIN

class vtkLineSource;
class vtkPolyDataMapper;

class VTKRENDERINGZSPACE_EXPORT vtkZSpaceRayActor : public vtkOpenGLActor
{
public:
  static vtkZSpaceRayActor* New();
  vtkTypeMacro(vtkZSpaceRayActor, vtkOpenGLActor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the state of the ray when nothing is picked.
   */
  void SetNoPick();

  /**
   * Set the state of the ray when something is picked.
   */
  void SetPick();

  ///@{
  /**
   * Get/Set the color of the ray when nothing is
   * picked.
   * Default is Red.
   */
  vtkGetVector3Macro(NoPickColor, double);
  vtkSetVector3Macro(NoPickColor, double);
  ///@}

  ///@{
  /**
   * Get/Set the color of the ray when something is
   * picked.
   * Default is Green.
   */
  vtkGetVector3Macro(PickColor, double);
  vtkSetVector3Macro(PickColor, double);
  ///@}

  ///@{
  /**
   * Get/Set the thickness of the line.
   * Default is 1.0.
   */
  vtkGetMacro(LineWidth, double);
  vtkSetMacro(LineWidth, double);
  ///@}

  ///@{
  /**
   * Get/Set the length of the ray.
   */
  vtkGetMacro(Length, double);
  vtkSetMacro(Length, double);
  ///@}

protected:
  vtkZSpaceRayActor();
  ~vtkZSpaceRayActor() override;

  vtkNew<vtkLineSource> LineSource;
  vtkNew<vtkPolyDataMapper> Mapper;

  double NoPickColor[3] = { 0.9, 0.3, 0.3 };
  double PickColor[3] = { 0.0, 1.0, 0.0 };
  double LineWidth = 1.5;
  double Length = 10.0;

private:
  vtkZSpaceRayActor(const vtkZSpaceRayActor&) = delete;
  void operator=(const vtkZSpaceRayActor&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif
