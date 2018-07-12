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

#include "vtkRenderingOSPRayModule.h" // For export macro
#include "vtkRendererNode.h"
#include <vector> // for ivars

#include "ospray/ospray.h" // for ospray handle types

class vtkInformationDoubleKey;
class vtkInformationDoubleVectorKey;
class vtkInformationIntegerKey;
class vtkInformationObjectBaseKey;
class vtkInformationStringKey;
class vtkMatrix4x4;
class vtkOSPRayRendererNodeInternals;
class vtkOSPRayMaterialLibrary;
class vtkRenderer;

class VTKRENDERINGOSPRAY_EXPORT vtkOSPRayRendererNode :
  public vtkRendererNode
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
  virtual void WriteLayer(unsigned char *buffer, float *zbuffer,
                          int buffx, int buffy, int layer);

  //state beyond rendering core...

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
  static void SetSamplesPerPixel(int, vtkRenderer *renderer);
  static int GetSamplesPerPixel(vtkRenderer *renderer);
  //@}

  //@{
  /**
   * When present on renderer, controls the number of ospray render calls
   * for each refresh.
   * default is 1
   */
  static vtkInformationIntegerKey* MAX_FRAMES();
  static void SetMaxFrames(int, vtkRenderer *renderer);
  static int GetMaxFrames(vtkRenderer *renderer);
  //@}

  //@{
  /**
   * Set the OSPRay renderer type to use (e.g. scivis vs. pathtracer)
   * default is scivis
   */
  static vtkInformationStringKey* RENDERER_TYPE();
  static void SetRendererType(std::string name, vtkRenderer *renderer);
  static std::string GetRendererType(vtkRenderer *renderer);
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
  static void SetAmbientSamples(int, vtkRenderer *renderer);
  static int GetAmbientSamples(vtkRenderer *renderer);
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
  static void SetCompositeOnGL(int, vtkRenderer *renderer);
  static int GetCompositeOnGL(vtkRenderer *renderer);
  //@}

  /**
   * World space direction of north pole for gradient and texture background.
   */
  static vtkInformationDoubleVectorKey* NORTH_POLE();
  //@{
  /**
   * Convenience method to set/get NORTH_POLE on a vtkRenderer.
   */
  static void SetNorthPole(double *, vtkRenderer *renderer);
  static double * GetNorthPole(vtkRenderer *renderer);
  //@}

  /**
   * World space direction of east pole for texture background.
   */
  static vtkInformationDoubleVectorKey* EAST_POLE();
  //@{
  /**
   * Convenience method to set/get EAST_POLE on a vtkRenderer.
   */
  static void SetEastPole(double *, vtkRenderer *renderer);
  static double * GetEastPole(vtkRenderer *renderer);
  //@}

  /**
   * Material Library attached to the renderer.
   */
  static vtkInformationObjectBaseKey* MATERIAL_LIBRARY();

  //@{
  /**
   * Convenience method to set/get Material library on a renderer.
   */
  static void SetMaterialLibrary(vtkOSPRayMaterialLibrary *, vtkRenderer *renderer);
  static vtkOSPRayMaterialLibrary* GetMaterialLibrary(vtkRenderer *renderer);
  //@}

  /**
   * Requested time to show in a renderer and to lookup in a temporal cache.
   */
  static vtkInformationDoubleKey* VIEW_TIME();
  //@{
  /**
   * Convenience method to set/get VIEW_TIME on a vtkRenderer.
   */
  static void SetViewTime(double , vtkRenderer *renderer);
  static double GetViewTime(vtkRenderer *renderer);
  //@}

  /**
   * Temporal cache size..
   */
  static vtkInformationIntegerKey* TIME_CACHE_SIZE();
  //@{
  /**
   * Convenience method to set/get TIME_CACHE_SIZE on a vtkRenderer.
   */
  static void SetTimeCacheSize(int , vtkRenderer *renderer);
  static int GetTimeCacheSize(vtkRenderer *renderer);
  //@}

  /**
   * Methods for other nodes to access
   */
  OSPModel GetOModel() { return this->OModel; }
  OSPRenderer GetORenderer() { return this->ORenderer; }
  void AddLight(OSPLight light) {
    this->Lights.push_back(light); }

  /**
   * Get the last rendered ColorBuffer
   */
  virtual unsigned char *GetBuffer() {
    return this->Buffer; }

  /**
   * Get the last rendered ZBuffer
   */
  virtual float *GetZBuffer() {
    return this->ZBuffer; }

  // if you want to traverse your children in a specific order
  // or way override this method
  virtual void Traverse(int operation) override;

  /**
   * Convenience method to get and downcast renderable.
   */
  vtkRenderer *GetRenderer();

protected:
  vtkOSPRayRendererNode();
  ~vtkOSPRayRendererNode();

  //internal structures
  unsigned char *Buffer;
  float *ZBuffer;

  OSPModel OModel;
  OSPRenderer ORenderer;
  OSPFrameBuffer OFrameBuffer;
  int ImageX, ImageY;
  std::vector<OSPLight> Lights;
  int NumActors;
  bool ComputeDepth;
  bool Accumulate;
  bool CompositeOnGL;
  float* ODepthBuffer;
  int AccumulateCount;
  vtkMTimeType AccumulateTime;
  vtkMatrix4x4 *AccumulateMatrix;
  vtkOSPRayRendererNodeInternals *Internal;

private:
  vtkOSPRayRendererNode(const vtkOSPRayRendererNode&) = delete;
  void operator=(const vtkOSPRayRendererNode&) = delete;
};

#endif
