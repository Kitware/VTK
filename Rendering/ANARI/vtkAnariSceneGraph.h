// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAnariSceneGraph
 * @brief   links vtkRenderers to ANARI
 *
 * This class acts as a root node managing an anari::Frame and everything
 * within it. The anari::Frame is the top-level object to render images using
 * ANARI, containing the anari::Camera, anari::World, and anari::Renderer all
 * from a particular anari::Device. vtkAnariSceneGraph expects to be given
 * the anari::Device and anari::Renderer externally, which are managed by
 * other classes which use vtkAnariSceneGraph (e.g. vtkAnariPass and
 * vtkAnariWindowNode). vtkAnariSceneGraph expects to be given a valid
 * anari::Device before any scene graph traversals occur.
 *
 *  @par Thanks:
 *  Kevin Griffin kgriffin@nvidia.com for creating and contributing the class
 *  and NVIDIA for supporting this work.
 */

#ifndef vtkAnariSceneGraph_h
#define vtkAnariSceneGraph_h

#include "vtkRendererNode.h"
#include "vtkRenderingAnariModule.h" // For export macro

#include <anari/anari_cpp.hpp> // For ANARI handles

VTK_ABI_NAMESPACE_BEGIN

class vtkInformationIntegerKey;
class vtkInformationDoubleKey;
class vtkInformationStringKey;
class vtkInformationDoubleVectorKey;
class vtkAnariSceneGraphInternals;
class vtkRenderer;
class vtkAnariDevice;

class VTKRENDERINGANARI_EXPORT vtkAnariSceneGraph : public vtkRendererNode
{
public:
  static vtkAnariSceneGraph* New();
  vtkTypeMacro(vtkAnariSceneGraph, vtkRendererNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Builds objects for this renderer.
   */
  void Build(bool prepass) override;

  /**
   * Traverse graph in ANARI's preferred order and render
   */
  void Render(bool prepass) override;

  /**
   * Invalidates cached rendering data.
   */
  void Invalidate(bool prepass) override;

  // state beyond rendering core...

  /**
   * When passing 'true', the renderer will skip actually rendering frame. This
   * is for when an application wants to externally use the anari::World in
   * their own non-VTK viewport using their own anari::Frame/Renderer/Camera,
   * but still wants VTK to manage the contents of the anari::World.
   */
  void SetUpdateWorldOnly(bool onlyUpdateWorld = false);

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
  anari::Device GetDeviceHandle() const;

  /**
   * Get the currently set ANARI renderer.
   */
  anari::Renderer GetRendererHandle() const;

  /**
   * Get the extensions supported by the current back-end device.
   */
  const anari::Extensions& GetAnariDeviceExtensions() const;
  //@}

  /**
   * Get the extensions supported by the current back-end device.
   */
  const char* const* GetAnariDeviceExtensionStrings() const;
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
   * Indicate that a new RenderTraversal of children needs to occur next frame
   */
  void InvalidateSceneStructure();

  /**
   * Reserve an Id which is unique to a render call.
   */
  int ReservePropId();

  /**
   * Convenience API to warn the user once per device per renderer per warning type.
   *
   * This saves the warning/error buffers to be filled each frame.
   */
  void WarningMacroOnce(vtkSmartPointer<vtkObject> caller, const std::string& warning);

protected:
  vtkAnariSceneGraph();
  ~vtkAnariSceneGraph() override;

  void InitAnariFrame(vtkRenderer* ren);
  void SetupAnariRendererParameters(vtkRenderer* ren);
  void InitAnariWorld();
  void UpdateAnariFrameSize();
  void UpdateAnariLights();
  void UpdateAnariSurfaces();
  void UpdateAnariVolumes();
  void CopyAnariFrameBufferData();
  void DebugOutputWorldBounds();
  void ResetReservedPropIds();

  vtkAnariSceneGraphInternals* Internal{ nullptr };

  vtkTimeStamp AnariSceneStructureModifiedMTime;
  vtkMTimeType AnariSceneConstructedMTime{ 0 };
  vtkTimeStamp AnariRendererModifiedTime;
  vtkMTimeType AnariRendererUpdatedTime{ 0 };

  std::map<std::string, std::vector<std::string>> IssuedWarnings;

private:
  vtkAnariSceneGraph(const vtkAnariSceneGraph&) = delete;
  void operator=(const vtkAnariSceneGraph&) = delete;

  void SetAnariDevice(vtkAnariDevice* ad, anari::Extensions e, const char* const* es);
  void SetAnariRenderer(anari::Renderer r);

  // only allow these classes to set the Anari device + renderer
  friend class vtkAnariPass;
};

VTK_ABI_NAMESPACE_END
#endif
