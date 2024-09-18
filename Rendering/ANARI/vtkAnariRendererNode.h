// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAnariRendererNode
 * @brief   links vtkRenderers to ANARI
 *
 * Translates vtkRenderer state into ANARI rendering calls
 *
 *  @par Thanks:
 *  Kevin Griffin kgriffin@nvidia.com for creating and contributing the class
 *  and NVIDIA for supporting this work.
 */

#ifndef vtkAnariRendererNode_h
#define vtkAnariRendererNode_h

#include "vtkRendererNode.h"
#include "vtkRenderingAnariModule.h" // For export macro

#include <anari/anari_cpp.hpp> // for external getter/setters
#include <vector>              // for ivars

VTK_ABI_NAMESPACE_BEGIN

class vtkInformationIntegerKey;
class vtkInformationDoubleKey;
class vtkInformationStringKey;
class vtkInformationDoubleVectorKey;
class vtkAnariRendererNodeInternals;
class vtkRenderer;

class VTKRENDERINGANARI_EXPORT vtkAnariRendererNode : public vtkRendererNode
{
public:
  static vtkAnariRendererNode* New();
  vtkTypeMacro(vtkAnariRendererNode, vtkRendererNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Builds objects for this renderer.
   */
  virtual void Build(bool prepass) override;

  /**
   * Traverse graph in ANARI's preferred order and render
   */
  virtual void Render(bool prepass) override;

  /**
   * Invalidates cached rendering data.
   */
  virtual void Invalidate(bool prepass) override;

  // state beyond rendering core...

  /**
   * Set the renderer subtype. Default is "default"
   */
  static vtkInformationStringKey* RENDERER_SUBTYPE();
  //@{
  /**
   * Convenience method to set/get RENDERER_SUBTYPE on a vtkRenderer.
   */
  static void SetRendererSubtype(vtkRenderer* renderer, const char* name);
  static const char* GetRendererSubtype(vtkRenderer* renderer);
  //@}

  /**
   * Set the number of frames to render which are accumulated to result in a
   * better converged image.
   */
  static vtkInformationIntegerKey* ACCUMULATION_COUNT();
  //@{
  /**
   * Convenience method to set/get ACCUMULATION_COUNT on a vtkRenderer.
   */
  static void SetAccumulationCount(vtkRenderer* renderer, int);
  static int GetAccumulationCount(vtkRenderer* renderer);
  //@}

  /**
   * used to make the renderer add ANARI's content onto GL rendered
   * content on the window
   */
  static vtkInformationIntegerKey* COMPOSITE_ON_GL();
  //@{
  /**
   * Convenience method to set/get COMPOSITE_ON_GL on a vtkRenderer.
   */
  static void SetCompositeOnGL(vtkRenderer* renderer, int);
  static int GetCompositeOnGL(vtkRenderer* renderer);
  //@}

  //@{
  /**
   * Methods to set generic parameteters on the underlying anari::Renderer object.
   */
  void SetAnariParameter(const char* param, bool);
  void SetAnariParameter(const char* param, int);
  void SetAnariParameter(const char* param, int, int);
  void SetAnariParameter(const char* param, int, int, int);
  void SetAnariParameter(const char* param, int, int, int, int);
  void SetAnariParameter(const char* param, float);
  void SetAnariParameter(const char* param, float, float);
  void SetAnariParameter(const char* param, float, float, float);
  void SetAnariParameter(const char* param, float, float, float, float);
  //@}

  //@{
  /**
   * Methods for other nodes to access
   */

  /**
   * Accessed by the AnariLightNode to add an ANARILight to the world.
   * Lights in ANARI are virtual objects that emit light into the world and
   * thus illuminate objects.
   */
  void AddLight(anari::Light);

  /**
   * Accessed by the AnariPolyDataMapperNode to an ANARISurface to the world.
   * Geometries are matched with appearance information through Surfaces.
   * These take a geometry, which defines the spatial representation, and
   * applies either full-object or per-primitive color and material information.
   */
  void AddSurface(anari::Surface);

  /**
   * Accessed by the AnariVolumeMapperNode to add Volumes to the world.
   * Volumes in ANARI represent volumetric objects (complementing surfaces),
   * enscapsulating spatial data as well as appearance information.
   */
  void AddVolume(anari::Volume);

  /**
   * Accessed by the AnariCameraNode to set the ANARICamera on the ANARIFrame.
   */
  void SetCamera(anari::Camera);

  /**
   * Get the ANARI back-end device. A device is an object which provides the
   * implementation of all ANARI API calls outside of libraries.
   */
  anari::Device GetAnariDevice();

  /**
   * Get the extensions supported by the current back-end device.
   */
  const anari::Extensions& GetAnariDeviceExtensions();
  //@}

  /**
   * Get the last rendered ColorBuffer
   */
  virtual const unsigned char* GetBuffer();

  /**
   * Get the last rendered ZBuffer
   */
  virtual const float* GetZBuffer();

  // if you want to traverse your children in a specific order
  // or way override this method
  virtual void Traverse(int operation) override;

  /**
   * Convenience method to get and downcast renderable.
   */
  vtkRenderer* GetRenderer();

  /**
   * Put my results into the correct place in the provided pixel buffer.
   */
  virtual void WriteLayer(unsigned char* buffer, float* zbuffer, int buffx, int buffy, int layer);

  /**
   * Get the last renderer color buffer as an OpenGL texture.
   */
  virtual int GetColorBufferTextureGL();

  /**
   * Get the last renderer depth buffer as an OpenGL texture.
   */
  virtual int GetDepthBufferTextureGL();

  /**
   * Indicate that a new RenderTraversal of children needs to occur next frame
   */
  void InvalidateSceneStructure();

  /**
   * Indicate that the parameters to the underlying ANARIRenderer need to be set.
   */
  static void InvalidateRendererParameters();

protected:
  vtkAnariRendererNode();
  ~vtkAnariRendererNode();

  void InitAnariFrame(vtkRenderer* ren);
  void InitAnariRenderer(vtkRenderer* ren);
  void SetupAnariRendererParameters(vtkRenderer* ren);
  void InitAnariWorld();
  void UpdateAnariFrameSize();
  void UpdateAnariLights();
  void UpdateAnariSurfaces();
  void UpdateAnariVolumes();
  void CopyAnariFrameBufferData();
  void DebugOutputWorldBounds();

  vtkAnariRendererNodeInternals* Internal{ nullptr };

  vtkTimeStamp AnariSceneStructureModifiedMTime;
  vtkMTimeType AnariSceneConstructedMTime{ 0 };
  static vtkTimeStamp AnariRendererModifiedTime;
  vtkMTimeType AnariRendererUpdatedTime{ 0 };

private:
  vtkAnariRendererNode(const vtkAnariRendererNode&) = delete;
  void operator=(const vtkAnariRendererNode&) = delete;

  void SetAnariDevice(anari::Device d, anari::Extensions e);

  // only allow these classes to set the Anari device on the scene graph
  friend class vtkAnariPass;
  friend class vtkAnariWindowNode;
};

VTK_ABI_NAMESPACE_END
#endif
