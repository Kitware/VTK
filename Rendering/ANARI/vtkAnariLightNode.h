// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAnariLightNode
 * @brief   links vtkLights to ANARI
 *
 * Translates vtkLight state into ANARILight state. Lights in ANARI are
 * virtual objects that emit light into the world and thus illuminate
 * objects.
 *
 * @par Thanks:
 * Kevin Griffin kgriffin@nvidia.com for creating and contributing the class
 * and NVIDIA for supporting this work.
 */

#ifndef vtkAnariLightNode_h
#define vtkAnariLightNode_h

#include "vtkLightNode.h"
#include "vtkRenderingAnariModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN

struct vtkAnariLightNodeInternals;
class vtkAnariSceneGraph;
class vtkInformationDoubleKey;
class vtkInformationIntegerKey;
class vtkLight;

class VTKRENDERINGANARI_EXPORT vtkAnariLightNode : public vtkLightNode
{
public:
  static vtkAnariLightNode* New();
  vtkTypeMacro(vtkAnariLightNode, vtkLightNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Ensure the right type of ANARICamera object is being held.
   */
  void Build(bool prepass) override;
  /**
   * Sync ANARICamera parameters with vtkCamera.
   */
  void Synchronize(bool prepass) override;
  /**
   * Make ANARI calls to render me.
   */
  void Render(bool prepass) override;

  /**
   * Invalidates cached rendering data.
   */
  void Invalidate(bool prepass) override;

  /**
   * A global multiplier to all ANARI lights.
   * default is 1.0
   */
  static vtkInformationDoubleKey* LIGHT_SCALE();

  ///@{
  /**
   * Convenience method to set/get LIGHT_SCALE on a vtkLight
   */
  static void SetLightScale(double, vtkLight*);
  static double GetLightScale(vtkLight*);
  ///@}

  // state beyond rendering core...

  /**
   * The radius setting, when > 0.0, produces soft shadows in the
   * path tracer.
   */
  static vtkInformationDoubleKey* RADIUS();

  //@{
  /**
   * Convenience method to set/get RADIUS on a vtkLight.
   */
  static void SetRadius(double, vtkLight*);
  static double GetRadius(vtkLight*);
  //@}

  /**
   * For cone-shaped lights, size (angle in radians) of the region
   * between the rim (of the illumination cone) and full intensity of
   * the spot; should be smaller than half of openingAngle
   */
  static vtkInformationDoubleKey* FALLOFF_ANGLE();

  //@{
  /**
   * Convenience method to set/get FALLOFF_ANGLE on a vtkLight.
   */
  static void SetFalloffAngle(double, vtkLight*);
  static double GetFalloffAngle(vtkLight*);
  //@}

protected:
  vtkAnariLightNode();
  ~vtkAnariLightNode() override;

private:
  vtkAnariLightNode(const vtkAnariLightNode&) = delete;
  void operator=(const vtkAnariLightNode&) = delete;

  void ClearLight();
  vtkLight* GetVtkLight() const;
  bool LightWasModified() const;

  vtkAnariLightNodeInternals* Internals{ nullptr };
};

VTK_ABI_NAMESPACE_END
#endif
