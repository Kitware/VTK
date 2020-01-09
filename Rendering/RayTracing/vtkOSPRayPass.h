/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOSPRayPass
 * @brief   a render pass that uses OSPRay instead of GL
 *
 * This is a render pass that can be put into a vtkRenderWindow which makes
 * it use OSPRay instead of OpenGL to render. Adding/Removing the pass
 * will swap back and forth between the two.
 *
 *  OSPRay MPI - OSPRay can use its own internal MPI layer to replicate
 *    the scene data across mpi processes and composite the image.
 *    This results in linear performance scaling and supports secondary
 *    rays.  To run in this mode, a special environment variable is supplied
 *    called VTKOSPRAY_ARGS where commandline flags can be inserted for
 *    OSPRay's init call.  As an example of this, below is a commandline
 *    for running paraview on localhost, but having OSPRay's rendering
 *    occur on 2 remote nodes.  ospray_mpi_worker is a separate application
 *    supplied with OSPRay binary packages or when built with MPI support
 *    from source.
 *    'mpirun -ppn 1 -hosts localhost VTKOSPRAY_ARGS="-osp:mpi"
 *      ./paraview : -hosts n1, n2 ./ospray_mpi_worker -osp:mpi'
 */

#ifndef vtkOSPRayPass_h
#define vtkOSPRayPass_h

#include "vtkRenderPass.h"
#include "vtkRenderingRayTracingModule.h" // For export macro

#include <string> // for std::string

class vtkCameraPass;
class vtkLightsPass;
class vtkOSPRayPassInternals;
class vtkOSPRayRendererNode;
class vtkOverlayPass;
class vtkRenderPassCollection;
class vtkSequencePass;
class vtkVolumetricPass;

class VTKRENDERINGRAYTRACING_EXPORT vtkOSPRayPass : public vtkRenderPass
{
public:
  static vtkOSPRayPass* New();
  vtkTypeMacro(vtkOSPRayPass, vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state s.
   */
  virtual void Render(const vtkRenderState* s) override;

  //@{
  /**
   * Tells the pass what it will render.
   */
  void SetSceneGraph(vtkOSPRayRendererNode*);
  vtkGetObjectMacro(SceneGraph, vtkOSPRayRendererNode);
  //@}

  /**
   * Called by the internals of this class
   */
  virtual void RenderInternal(const vtkRenderState* s);

  //@{
  /**
   * Wrapper around ospray's init and shutdown that protect
   * with a reference count.
   */
  //@}
  static void RTInit();
  static void RTShutdown();

  /**
   * A run time query to see if a particular backend is available.
   * Eg. "OSPRay raycaster", "OSPRay pathtracer" or "OptiX pathtracer".
   */
  static bool IsBackendAvailable(const char* name);

protected:
  /**
   * Default constructor.
   */
  vtkOSPRayPass();

  /**
   * Destructor.
   */
  ~vtkOSPRayPass() override;

  vtkOSPRayRendererNode* SceneGraph;
  vtkCameraPass* CameraPass;
  vtkLightsPass* LightsPass;
  vtkOverlayPass* OverlayPass;
  vtkVolumetricPass* VolumetricPass;
  vtkSequencePass* SequencePass;
  vtkRenderPassCollection* RenderPassCollection;

private:
  vtkOSPRayPass(const vtkOSPRayPass&) = delete;
  void operator=(const vtkOSPRayPass&) = delete;

  vtkOSPRayPassInternals* Internal;
  std::string PreviousType;
  static int RTDeviceRefCount;
};

#endif
