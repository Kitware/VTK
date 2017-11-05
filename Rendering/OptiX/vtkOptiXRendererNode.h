/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOptiXRendererNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOptiXRendererNode
 * @brief   links vtkRenderers to OptiX
 *
 * Translates vtkRenderer state into OptiX rendering calls
*/

#ifndef vtkOptiXRendererNode_h
#define vtkOptiXRendererNode_h

#include "vtkRenderingOptiXModule.h" // For export macro
#include "vtkRendererNode.h"

#include <vector> // for ivars

class vtkRenderer;
class vtkInformationIntegerKey;
class vtkOptiXPtxLoader;

struct vtkOptiXRendererNodeInternals;

namespace vtkopt
{
  struct Light;
}

namespace optix
{
  class ContextObj;
  class GeometryGroupObj;
}

class VTKRENDERINGOPTIX_EXPORT vtkOptiXRendererNode : public vtkRendererNode
{
public:
  static vtkOptiXRendererNode* New();
  vtkTypeMacro(vtkOptiXRendererNode, vtkRendererNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Builds myself.
   */
  virtual void Build(bool prepass);

  /**
   * Traverse graph in OptiX's preferred order and render.
   */
  virtual void Render(bool prepass);

  /**
   * Put my results into the correct place in the provided pixel buffer.
   */
  virtual void WriteLayer(unsigned char *buffer, float *zbuffer,
          int buffx, int buffy, int layer);

  //state beyond rendering core...

  /**
   * When present on renderer, controls the number of primary rays
   * shot per pixel.
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
   * When present on renderer, controls the number of OptiX render calls
   * for each refresh.
   * default is 1
   */
  static vtkInformationIntegerKey* MAX_FRAMES();
  static void SetMaxFrames(int, vtkRenderer *renderer);
  static int GetMaxFrames(vtkRenderer *renderer);
  //@}

  //@{
  /**
   * When present on renderer, controls the number of ambient occlusion
   * samples shot per hit.
   * default is 4
   */
  static vtkInformationIntegerKey* AMBIENT_SAMPLES();
  static void SetAmbientSamples(int, vtkRenderer *renderer);
  static int GetAmbientSamples(vtkRenderer *renderer);
  //@}

  /**
   * Get the last rendered ColorBuffer
   */
  virtual unsigned char *GetBuffer() { return this->Buffer; }

  /**
   * Get the last rendered ZBuffer
   */
  virtual float *GetZBuffer() { return this->ZBuffer; }

  /**
   * Get the OptiX Context
   */
  virtual optix::ContextObj* GetOptiXContext();

  /**
   * Get the top level GeometryGroup
   */
  virtual optix::GeometryGroupObj* GetOptiXGeometryGroup();

  /**
  * Get the OptiX Ptx loader
  */
  vtkOptiXPtxLoader* GetOptiXPtxLoader();

  /**
   * Add a Light
   */
  virtual void AddLight(const vtkopt::Light& light);

  /**
   * If you want to traverse your children in a specific order
   * or way override this method.
   */
  virtual void Traverse(int operation);

  /**
   * Synchronizes render output by recreating depth/color buffer.
   */
  virtual void Synchronize(bool prepass);

protected:
  vtkOptiXRendererNode();
  ~vtkOptiXRendererNode();

  //internal structures
  unsigned char *Buffer;
  float *ZBuffer;

  vtkOptiXRendererNodeInternals* Internals;
  vtkOptiXPtxLoader* OptiXPtxLoader;

  std::vector<vtkopt::Light> Lights;

  int NumActors;

private:
  vtkOptiXRendererNode(const vtkOptiXRendererNode&) = delete;
  void operator=(const vtkOptiXRendererNode&) = delete;

  int ImageX, ImageY;

  mutable float Cached_bgColor[3];
  mutable int Cached_useShadows;
  mutable int Cached_samplesPerPixel;
  mutable bool ContextValidated;
  mutable int Cached_AOSamples;
};

#endif
