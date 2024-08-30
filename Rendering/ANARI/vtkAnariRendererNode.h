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
   * Set the library name. This will look for a shared library named
   * anari_library_[name](e.g., anari_library_visrtx).
   * The default uses the environment library which reads ANARI_LIBRARY
   * as an environment variable to get the library name to load.
   */
  static vtkInformationStringKey* LIBRARY_NAME();
  //@{
  /**
   * Convenience method to set/get LIBRARY_NAME on a vtkRenderer.
   * "environment" is returned if no library name is set.
   */
  static void SetLibraryName(vtkRenderer* renderer, const char* name);
  static const char* GetLibraryName(vtkRenderer* renderer);
  //@}

  /**
   * Set the back-end device subtype. Default is "default"
   */
  static vtkInformationStringKey* DEVICE_SUBTYPE();
  //@{
  /**
   * Convenience method to set/get DEVICE_SUBTYPE on a vtkRenderer.
   */
  static void SetDeviceSubtype(vtkRenderer* renderer, const char* name);
  static const char* GetDeviceSubtype(vtkRenderer* renderer);
  //@}

  /**
   * Set the debug library name. This will look for a shared library named
   * anari_library_[debug_name](e.g., anari_library_debug).
   */
  static vtkInformationStringKey* DEBUG_LIBRARY_NAME();
  //@{
  /**
   * Convenience method to set/get DEBUG_LIBRARY_NAME on a vtkRenderer.
   * The default is "debug".
   */
  static void SetDebugLibraryName(vtkRenderer* renderer, const char* name);
  static const char* GetDebugLibraryName(vtkRenderer* renderer);
  //@}

  /**
   * Set the debug device subtype. Default is "debug"
   */
  static vtkInformationStringKey* DEBUG_DEVICE_SUBTYPE();
  //@{
  /**
   * Convenience method to set/get DEBUG_DEVICE_SUBTYPE on a vtkRenderer.
   * Default is "debug".
   */
  static void SetDebugDeviceSubtype(vtkRenderer* renderer, const char* name);
  static const char* GetDebugDeviceSubtype(vtkRenderer* renderer);
  //@}

  /**
   * Set the debug device output directory. Default is nullptr.
   */
  static vtkInformationStringKey* DEBUG_DEVICE_DIRECTORY();
  //@{
  /**
   * Convenience method to set/get DEBUG_DEVICE_DIRECTORY on a vtkRenderer.
   * Default is nullptr.
   */
  static void SetDebugDeviceDirectory(vtkRenderer* renderer, const char* name);
  static const char* GetDebugDeviceDirectory(vtkRenderer* renderer);
  //@}

  /**
   * Set the debug device trace mode Default is "code".
   */
  static vtkInformationStringKey* DEBUG_DEVICE_TRACE_MODE();
  //@{
  /**
   * Convenience method to set/get DEBUG_DEVICE_TRACE_MODE on a vtkRenderer.
   * Default is "code"
   */
  static void SetDebugDeviceTraceMode(vtkRenderer* renderer, const char* name);
  static const char* GetDebugDeviceTraceMode(vtkRenderer* renderer);
  //@}

  /**
   * When present on renderer, controls if the rendering back-end should be wrapped
   * in a debug device to support trace dumps.
   */
  static vtkInformationIntegerKey* USE_DEBUG_DEVICE();
  //@{
  /**
   * Convenience method to set/get USE_DEBUG_DEVICE on a vtkRenderer.
   * Default value is 0.
   */
  static void SetUseDebugDevice(vtkRenderer* renderer, int);
  static int GetUseDebugDevice(vtkRenderer* renderer);
  //@}

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
   * Get the ANARI back-end device subtype name.
   */
  std::string GetAnariDeviceName();

  /**
   * Get the ANARI library. ANARI libraries are the mechanism that applications
   * use to manage API device implementations. Libraries are solely responsible
   * for creating instances of devices.
   */
  anari::Library GetAnariLibrary();

  /**
   * Get the extensions supported by the current back-end device.
   */
  anari::Extensions GetAnariDeviceExtensions();
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
};

VTK_ABI_NAMESPACE_END
#endif
