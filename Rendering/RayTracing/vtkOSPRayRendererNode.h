/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayRendererNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOSPRayRendererNode
 * @brief   links vtkRenderers to OSPRay
 *
 * Translates vtkRenderer state into OSPRay rendering calls
 */

#ifndef vtkOSPRayRendererNode_h
#define vtkOSPRayRendererNode_h

#include "vtkRendererNode.h"
#include "vtkRenderingRayTracingModule.h" // For export macro
#include <vector>                         // for ivars

#include "RTWrapper/RTWrapper.h" // for handle types

#ifdef VTKOSPRAY_ENABLE_DENOISER
#include <OpenImageDenoise/oidn.hpp> // for denoiser structures
#endif

class vtkInformationDoubleKey;
class vtkInformationDoubleVectorKey;
class vtkInformationIntegerKey;
class vtkInformationObjectBaseKey;
class vtkInformationStringKey;
class vtkMatrix4x4;
class vtkOSPRayRendererNodeInternals;
class vtkOSPRayMaterialLibrary;
class vtkRenderer;

class VTKRENDERINGRAYTRACING_EXPORT vtkOSPRayRendererNode : public vtkRendererNode
{
public:
  static vtkOSPRayRendererNode* New();
  vtkTypeMacro(vtkOSPRayRendererNode, vtkRendererNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Builds myself.
   */
  virtual void Build(bool prepass) override;

  /**
   * Traverse graph in ospray's preferred order and render
   */
  virtual void Render(bool prepass) override;

  /**
   * Invalidates cached rendering data.
   */
  virtual void Invalidate(bool prepass) override;

  /**
   * Put my results into the correct place in the provided pixel buffer.
   */
  virtual void WriteLayer(unsigned char* buffer, float* zbuffer, int buffx, int buffy, int layer);

  // state beyond rendering core...

  /**
   * When present on renderer, controls the number of primary rays
   * shot per pixel
   * default is 1
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
   * When present on renderer, samples are clamped to this value before they
   * are accumulated into the framebuffer
   * default is 2.0
   */
  static vtkInformationDoubleKey* MAX_CONTRIBUTION();

  //@{
  /**
   * Convenience method to set/get MAX_CONTRIBUTION on a vtkRenderer.
   */
  static void SetMaxContribution(double, vtkRenderer* renderer);
  static double GetMaxContribution(vtkRenderer* renderer);
  //@}

  /**
   * When present on renderer, controls the maximum ray recursion depth
   * default is 20
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
   * When present on renderer, sample contributions below this value will be
   * neglected to speedup rendering
   * default is 0.01
   */
  static vtkInformationDoubleKey* MIN_CONTRIBUTION();

  //@{
  /**
   * Convenience method to set/get MIN_CONTRIBUTION on a vtkRenderer.
   */
  static void SetMinContribution(double, vtkRenderer* renderer);
  static double GetMinContribution(vtkRenderer* renderer);
  //@}

  /**
   * When present on renderer, controls the ray recursion depth at which to
   * start Russian roulette termination
   * default is 5
   */
  static vtkInformationIntegerKey* ROULETTE_DEPTH();

  //@{
  /**
   * Convenience method to set/get ROULETTE_DEPTH on a vtkRenderer.
   */
  static void SetRouletteDepth(int, vtkRenderer* renderer);
  static int GetRouletteDepth(vtkRenderer* renderer);
  //@}

  /**
   * When present on renderer, controls the threshold for adaptive accumulation
   * default is 0.3
   */
  static vtkInformationDoubleKey* VARIANCE_THRESHOLD();

  //@{
  /**
   * Convenience method to set/get VARIANCE_THRESHOLD on a vtkRenderer.
   */
  static void SetVarianceThreshold(double, vtkRenderer* renderer);
  static double GetVarianceThreshold(vtkRenderer* renderer);
  //@}

  //@{
  /**
   * When present on renderer, controls the number of ospray render calls
   * for each refresh.
   * default is 1
   */
  static vtkInformationIntegerKey* MAX_FRAMES();
  static void SetMaxFrames(int, vtkRenderer* renderer);
  static int GetMaxFrames(vtkRenderer* renderer);
  //@}

  //@{
  /**
   * Set the OSPRay renderer type to use (e.g. scivis vs. pathtracer)
   * default is scivis
   */
  static vtkInformationStringKey* RENDERER_TYPE();
  static void SetRendererType(std::string name, vtkRenderer* renderer);
  static std::string GetRendererType(vtkRenderer* renderer);
  //@}

  /**
   * When present on renderer, controls the number of ambient occlusion
   * samples shot per hit.
   * default is 4
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
   * used to make the renderer add ospray's content onto GL rendered
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

  /**
   * World space direction of north pole for gradient and texture background.
   */
  static vtkInformationDoubleVectorKey* NORTH_POLE();
  //@{
  /**
   * Convenience method to set/get NORTH_POLE on a vtkRenderer.
   */
  static void SetNorthPole(double*, vtkRenderer* renderer);
  static double* GetNorthPole(vtkRenderer* renderer);
  //@}

  /**
   * World space direction of east pole for texture background.
   */
  static vtkInformationDoubleVectorKey* EAST_POLE();
  //@{
  /**
   * Convenience method to set/get EAST_POLE on a vtkRenderer.
   */
  static void SetEastPole(double*, vtkRenderer* renderer);
  static double* GetEastPole(vtkRenderer* renderer);
  //@}

  /**
   * Material Library attached to the renderer.
   */
  static vtkInformationObjectBaseKey* MATERIAL_LIBRARY();

  //@{
  /**
   * Convenience method to set/get Material library on a renderer.
   */
  static void SetMaterialLibrary(vtkOSPRayMaterialLibrary*, vtkRenderer* renderer);
  static vtkOSPRayMaterialLibrary* GetMaterialLibrary(vtkRenderer* renderer);
  //@}

  /**
   * Requested time to show in a renderer and to lookup in a temporal cache.
   */
  static vtkInformationDoubleKey* VIEW_TIME();
  //@{
  /**
   * Convenience method to set/get VIEW_TIME on a vtkRenderer.
   */
  static void SetViewTime(double, vtkRenderer* renderer);
  static double GetViewTime(vtkRenderer* renderer);
  //@}

  /**
   * Temporal cache size..
   */
  static vtkInformationIntegerKey* TIME_CACHE_SIZE();
  //@{
  /**
   * Convenience method to set/get TIME_CACHE_SIZE on a vtkRenderer.
   */
  static void SetTimeCacheSize(int, vtkRenderer* renderer);
  static int GetTimeCacheSize(vtkRenderer* renderer);
  //@}

  /**
   * Methods for other nodes to access
   */
  OSPModel GetOModel() { return this->OModel; }
  OSPRenderer GetORenderer() { return this->ORenderer; }
  void AddLight(OSPLight light) { this->Lights.push_back(light); }

  /**
   * Get the last rendered ColorBuffer
   */
  virtual void* GetBuffer() { return this->Buffer.data(); }

  /**
   * Get the last rendered ZBuffer
   */
  virtual float* GetZBuffer() { return this->ZBuffer.data(); }

  // Get the last renderer color buffer as an OpenGL texture.
  virtual int GetColorBufferTextureGL() { return this->ColorBufferTex; }

  // Get the last renderer depth buffer as an OpenGL texture.
  virtual int GetDepthBufferTextureGL() { return this->DepthBufferTex; }

  // if you want to traverse your children in a specific order
  // or way override this method
  virtual void Traverse(int operation) override;

  /**
   * Convenience method to get and downcast renderable.
   */
  static vtkOSPRayRendererNode* GetRendererNode(vtkViewNode*);
  vtkRenderer* GetRenderer();
  RTW::Backend* GetBackend();

  /**
   * Accumulation threshold when above which denoising kicks in.
   */
  static vtkInformationIntegerKey* DENOISER_THRESHOLD();
  //@{
  /**
   * Convenience method to set/get DENOISER_THRESHOLD on a vtkRenderer.
   */
  static void SetDenoiserThreshold(int, vtkRenderer* renderer);
  static int GetDenoiserThreshold(vtkRenderer* renderer);
  //@}

  //@{
  /**
   * Enable denoising (if supported).
   */
  static vtkInformationIntegerKey* ENABLE_DENOISER();
  /**
   * Convenience method to set/get ENABLE_DENOISER on a vtkRenderer.
   */
  static void SetEnableDenoiser(int, vtkRenderer* renderer);
  static int GetEnableDenoiser(vtkRenderer* renderer);
  //@}

  //@{
  /**
   * Control use of the path tracer backplate and environmental background.
   * 0 means neither is shown, 1 means only backplate is shown,
   * 2 (the default) means only environment is shown, 3 means that
   * both are enabled and therefore backblate shows on screen but
   * actors acquire color from the environment.
   */
  static vtkInformationIntegerKey* BACKGROUND_MODE();
  static void SetBackgroundMode(int, vtkRenderer* renderer);
  static int GetBackgroundMode(vtkRenderer* renderer);
  //@}
protected:
  vtkOSPRayRendererNode();
  ~vtkOSPRayRendererNode() override;

  /**
   * Denoise the colors stored in ColorBuffer and put into Buffer
   */
  void Denoise();

  // internal structures
#ifdef VTKOSPRAY_ENABLE_DENOISER
  std::vector<float> Buffer;
#else
  std::vector<unsigned char> Buffer;
#endif
  std::vector<float> ZBuffer;

  int ColorBufferTex;
  int DepthBufferTex;

  OSPModel OModel;
  OSPRenderer ORenderer;
  OSPFrameBuffer OFrameBuffer;
  OSPData OLightArray;
  int ImageX, ImageY;
  std::vector<OSPLight> Lights;
  int NumActors;
  bool ComputeDepth;
  bool Accumulate;
  bool CompositeOnGL;
  std::vector<float> ODepthBuffer;
  int AccumulateCount;
  int ActorCount;
  vtkMTimeType AccumulateTime;
  vtkMatrix4x4* AccumulateMatrix;
  vtkOSPRayRendererNodeInternals* Internal;
  std::string PreviousType;

#ifdef VTKOSPRAY_ENABLE_DENOISER
  oidn::DeviceRef DenoiserDevice;
  oidn::FilterRef DenoiserFilter;
#endif
  bool DenoiserDirty{ true };
  std::vector<osp::vec4f> ColorBuffer;
  std::vector<osp::vec3f> NormalBuffer;
  std::vector<osp::vec3f> AlbedoBuffer;
  std::vector<osp::vec4f> DenoisedBuffer;

private:
  vtkOSPRayRendererNode(const vtkOSPRayRendererNode&) = delete;
  void operator=(const vtkOSPRayRendererNode&) = delete;
};

#endif
