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
// .NAME vtkOSPRayRendererNode - links vtkRenderers to OSPRay
// .SECTION Description
// Translates vtkRenderer state into OSPRay rendering calls

#ifndef vtkOSPRayRendererNode_h
#define vtkOSPRayRendererNode_h

#include "vtkRenderingOSPRayModule.h" // For export macro
#include "vtkRendererNode.h"

#include <vector> // for ivars

class vtkRenderer;
class vtkInformationIntegerKey;
// ospray forward decs so that someone does not need to include ospray.h
namespace osp {
struct Model;
struct Renderer;
struct Light;
}
typedef osp::Model *OSPModel;
typedef osp::Renderer *OSPRenderer;
typedef osp::Light *OSPLight;

class VTKRENDERINGOSPRAY_EXPORT vtkOSPRayRendererNode :
  public vtkRendererNode
{
public:
  static vtkOSPRayRendererNode* New();
  vtkTypeMacro(vtkOSPRayRendererNode, vtkRendererNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  //Description:
  //Builds myself.
  virtual void Build(bool prepass);

  //Description:
  //Traverse graph in ospray's prefered order and render
  virtual void Render(bool prepass);

  //Description:
  //Put my results into the correct place in the provided pixel buffer.
  virtual void WriteLayer(unsigned char *buffer, float *zbuffer,
                          int buffx, int buffy, int layer);

  //state beyond rendering core...

  //Description:
  //When present on renderer, controls the number of primary rays
  //shot per pixel
  //default is 1
  static vtkInformationIntegerKey* SAMPLES_PER_PIXEL();

  //Description:
  //Convenience method to set/get SAMPLES_PER_PIXEL on a vtkRenderer.
  static void SetSamplesPerPixel(int, vtkRenderer *renderer);
  static int GetSamplesPerPixel(vtkRenderer *renderer);

  //Description:
  //When present on renderer, controls the number of ospray render calls
  //for each refresh.
  //default is 1
  static vtkInformationIntegerKey* MAX_FRAMES();
  static void SetMaxFrames(int, vtkRenderer *renderer);
  static int GetMaxFrames(vtkRenderer *renderer);

  //Description:
  //When present on renderer, controls the number of ambient occlusion
  //samples shot per hit.
  //default is 4
  static vtkInformationIntegerKey* AMBIENT_SAMPLES();
  //Description:
  //Convenience method to set/get SAMPLES_PER_PIXEL on a vtkRenderer.
  static void SetAmbientSamples(int, vtkRenderer *renderer);
  static int GetAmbientSamples(vtkRenderer *renderer);

  // Description:
  // Methods for other nodes to access
  OSPModel GetOModel() { return this->OModel; }
  OSPRenderer GetORenderer() { return this->ORenderer; }
  void AddLight(OSPLight light) {
    this->Lights.push_back(light); }

  // Description:
  // Get the last rendered ColorBuffer
  virtual unsigned char *GetBuffer() {
    return this->Buffer; }

  // Description:
  // Get the last rendered ZBuffer
  virtual float *GetZBuffer() {
    return this->ZBuffer; }

  // if you want to traverse your children in a specific order
  // or way override this method
  virtual void Traverse(int operation);

  void SetMaxDepthTexture(void *dt);

protected:
  vtkOSPRayRendererNode();
  ~vtkOSPRayRendererNode();

  //internal structures
  unsigned char *Buffer;
  float *ZBuffer;

  OSPModel OModel;
  OSPRenderer ORenderer;
  std::vector<OSPLight> Lights;
  void *MaxDepth;
  int NumActors;
private:
  vtkOSPRayRendererNode(const vtkOSPRayRendererNode&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOSPRayRendererNode&) VTK_DELETE_FUNCTION;
};

#endif
