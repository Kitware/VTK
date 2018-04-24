/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSkybox.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSkybox
 * @brief Renders a skybox environment
 *
 * You must provide a texture cube map using the SetTexture method.
 */

#ifndef vtkSkybox_h
#define vtkSkybox_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkActor.h"

class VTKRENDERINGCORE_EXPORT vtkSkybox: public vtkActor
{
public:
  static vtkSkybox* New();
  vtkTypeMacro(vtkSkybox, vtkActor)
  void PrintSelf(ostream &os, vtkIndent indent) override;

  /**
   * Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax). (The
   * method GetBounds(double bounds[6]) is available from the superclass.)
   */
  using Superclass::GetBounds;
  double *GetBounds() override;

  enum Projection
  {
    Cube,
    Sphere,
    Floor,
  };
  vtkGetMacro(Projection, int);
  vtkSetMacro(Projection, int);
  void SetProjectionToCube() { this->SetProjection(vtkSkybox::Cube); }
  void SetProjectionToSphere() {this->SetProjection(vtkSkybox::Sphere); }
  void SetProjectionToFloor() {this->SetProjection(vtkSkybox::Floor); }

  /**
   * Set/Get the plane equation for the floor.
   */
  vtkSetVector4Macro(FloorPlane, float);
  vtkGetVector4Macro(FloorPlane, float);

  vtkSetVector3Macro(FloorRight, float);
  vtkGetVector3Macro(FloorRight, float);

protected:
  vtkSkybox();
  ~vtkSkybox() override;

  int Projection;
  float FloorPlane[4];
  float FloorRight[3];

private:
  vtkSkybox(const vtkSkybox&) = delete;
  void operator=(const vtkSkybox&) = delete;
};

#endif // vtkSkybox_h
