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
   * When present on renderer, controls if a denoiser is used during
   * rendering.
   */
  static vtkInformationIntegerKey* USE_DENOISER();
  //@{
  /**
   * Convenience method to set/get USE_DENOISER on a vtkRenderer.
   * Default value is 0.
   */
  static void SetUseDenoiser(int, vtkRenderer* renderer);
  static int GetUseDenoiser(vtkRenderer* renderer);
  //@}

  /**
   * When present on renderer, controls the number of primary rays
   * shot per pixel.default is 1
   */
  static vtkInformationIntegerKey* SAMPLES_PER_PIXEL();
  //@{
  /**
   * Convenience method to set/get SAMPLES_PER_PIXEL on a vtkRenderer.
   */
  static void SetSamplesPerPixel(int, vtkRenderer* renderer);
  static int GetSamplesPerPixel(vtkRenderer* renderer);
  //@}

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
  static void SetLibraryName(const char* name, vtkRenderer* renderer);
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
  static void SetDeviceSubtype(const char* name, vtkRenderer* renderer);
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
  static void SetDebugLibraryName(const char* name, vtkRenderer* renderer);
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
  static void SetDebugDeviceSubtype(const char* name, vtkRenderer* renderer);
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
  static void SetDebugDeviceDirectory(const char* name, vtkRenderer* renderer);
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
  static void SetDebugDeviceTraceMode(const char* name, vtkRenderer* renderer);
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
  static void SetUseDebugDevice(int, vtkRenderer* renderer);
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
  static void SetRendererSubtype(const char* name, vtkRenderer* renderer);
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
  static void SetAccumulationCount(int, vtkRenderer* renderer);
  static int GetAccumulationCount(vtkRenderer* renderer);
  //@}

  /**
   * When present on renderer, controls the number of ambient occlusion
   * samples shot per hit.
   * Default is 0
   */
  static vtkInformationIntegerKey* AMBIENT_SAMPLES();
  //@{
  /**
   * Convenience method to set/get AMBIENT_SAMPLES on a vtkRenderer.
   */
  static void SetAmbientSamples(int, vtkRenderer* renderer);
  static int GetAmbientSamples(vtkRenderer* renderer);
  //@}

  /**
   * When present on renderer, controls the light falloff value used by
   * the back-end renderer.
   * Default is 1.
   */
  static vtkInformationDoubleKey* LIGHT_FALLOFF();
  //@{
  /**
   * Convenience method to set/get LIGHT_FALLOFF on a vtkRenderer.
   */
  static void SetLightFalloff(double, vtkRenderer* renderer);
  static double GetLightFalloff(vtkRenderer* renderer);
  //@}

  /**
   * When present on renderer, controls the ambient color used
   * by the back-end renderer.
   * Default is white {1,1,1}.
   */
  static vtkInformationDoubleVectorKey* AMBIENT_COLOR();
  //@{
  /**
   * Convenience method to set/get AMBIENT_COLOR on a vtkRenderer.
   */
  static void SetAmbientColor(double*, vtkRenderer* renderer);
  static double* GetAmbientColor(vtkRenderer* renderer);
  //@}

  /**
   * When present on renderer, controls the ambient intensity value used
   * by the back-end renderer.
   * Default is 1.
   */
  static vtkInformationDoubleKey* AMBIENT_INTENSITY();
  //@{
  /**
   * Convenience method to set/get AMBIENT_INTENSITY on a vtkRenderer.
   */
  static void SetAmbientIntensity(double, vtkRenderer* renderer);
  static double GetAmbientIntensity(vtkRenderer* renderer);
  //@}

  /**
   * When present on renderer, controls the max depth value used
   * by the back-end renderer.
   * Default is 0.
   */
  static vtkInformationIntegerKey* MAX_DEPTH();
  //@{
  /**
   * Convenience method to set/get MAX_DEPTH on a vtkRenderer.
   */
  static void SetMaxDepth(int, vtkRenderer* renderer);
  static int GetMaxDepth(vtkRenderer* renderer);
  //@}

  /**
   * When present on renderer, controls the R value used by the back-end
   * renderer.
   * Default is 1.
   */
  static vtkInformationDoubleKey* R_VALUE();
  //@{
  /**
   * Convenience method to set/get R_VALUE on a vtkRenderer.
   */
  static void SetROptionValue(double, vtkRenderer* renderer);
  static double GetROptionValue(vtkRenderer* renderer);
  //@}

  /**
   * Set the debug method. nullptr is returned if not set.
   */
  static vtkInformationStringKey* DEBUG_METHOD();
  //@{
  /**
   * Convenience method to set/get DEBUG_METHOD on a vtkRenderer.
   */
  static void SetDebugMethod(const char*, vtkRenderer* renderer);
  static const char* GetDebugMethod(vtkRenderer* renderer);
  //@}

  /**
   * Set the USD output directory. nullptr is returned if not set.
   */
  static vtkInformationStringKey* USD_DIRECTORY();
  //@{
  /**
   * Convenience method to set/get USD_DIRECTORY on a vtkRenderer.
   */
  static void SetUsdDirectory(const char*, vtkRenderer* renderer);
  static const char* GetUsdDirectory(vtkRenderer* renderer);
  //@}

  /**
   * Sets the output USD at anariCommit flag for the USD back-end.
   * Default is 0.
   */
  static vtkInformationIntegerKey* USD_COMMIT();
  //@{
  /**
   * Convenience method to set/get USD_COMMIT on a vtkRenderer.
   */
  static void SetUsdAtCommit(int, vtkRenderer* renderer);
  static int GetUsdAtCommit(vtkRenderer* renderer);
  //@}

  /**
   * Sets the output USD in binary format flag for the USD back-end.
   * Default is 1.
   */
  static vtkInformationIntegerKey* USD_OUTPUT_BINARY();
  //@{
  /**
   * Convenience method to set/get USD_OUTPUT_BINARY on a vtkRenderer.
   */
  static void SetUsdOutputBinary(int, vtkRenderer* renderer);
  static int GetUsdOutputBinary(vtkRenderer* renderer);
  //@}

  /**
   * Sets the output USD material objects flag for the USD back-end.
   * Default is 1.
   */
  static vtkInformationIntegerKey* USD_OUTPUT_MATERIAL();
  //@{
  /**
   * Convenience method to set/get USD_OUTPUT_MATERIAL on a vtkRenderer.
   */
  static void SetUsdOutputMaterial(int, vtkRenderer* renderer);
  static int GetUsdOutputMaterial(vtkRenderer* renderer);
  //@}

  /**
   * Sets the output USD preview surface prims for material objects flag
   * for the USD back-end.
   * Default is 1.
   */
  static vtkInformationIntegerKey* USD_OUTPUT_PREVIEW();
  //@{
  /**
   * Convenience method to set/get USD_OUTPUT_PREVIEW on a vtkRenderer.
   */
  static void SetUsdOutputPreviewSurface(int, vtkRenderer* renderer);
  static int GetUsdOutputPreviewSurface(vtkRenderer* renderer);
  //@}

  /**
   * Sets the output USD mdl shader prims for material objects flag for the
   * USD back-end.
   * Default is 1.
   */
  static vtkInformationIntegerKey* USD_OUTPUT_MDL();
  //@{
  /**
   * Convenience method to set/get USD_OUTPUT_MDL on a vtkRenderer.
   */
  static void SetUsdOutputMDL(int, vtkRenderer* renderer);
  static int GetUsdOutputMDL(vtkRenderer* renderer);
  //@}

  /**
   * Sets the output USD mdl colors for material objects flag for the USD
   * back-end.
   * Default is 1.
   */
  static vtkInformationIntegerKey* USD_OUTPUT_MDLCOLORS();
  //@{
  /**
   * Convenience method to set/get USD_OUTPUT_MDLCOLORS on a vtkRenderer.
   */
  static void SetUsdOutputMDLColors(int, vtkRenderer* renderer);
  static int GetUsdOutputMDLColors(vtkRenderer* renderer);
  //@}

  /**
   * Sets the output USD display colors flag for the USD back-end.
   * Default is 1.
   */
  static vtkInformationIntegerKey* USD_OUTPUT_DISPLAYCOLORS();
  //@{
  /**
   * Convenience method to set/get USD_OUTPUT_DISPLAYCOLORS on a vtkRenderer.
   */
  static void SetUsdOutputDisplayColors(int, vtkRenderer* renderer);
  static int GetUsdOutputDisplayColors(vtkRenderer* renderer);
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
  static void SetCompositeOnGL(int, vtkRenderer* renderer);
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
