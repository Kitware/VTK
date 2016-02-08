/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLightNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLightNode - vtkViewNode specialized for vtkLights
// .SECTION Description
// State storage and graph traversal for vtkLight

#ifndef vtkLightNode_h
#define vtkLightNode_h

#include "vtkRenderingSceneGraphModule.h" // For export macro
#include "vtkViewNode.h"

class VTKRENDERINGSCENEGRAPH_EXPORT vtkLightNode :
  public vtkViewNode
{
public:
  static vtkLightNode* New();
  vtkTypeMacro(vtkLightNode, vtkViewNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  //Description:
  //Build containers for our (not existent) child nodes.
  virtual void BuildSelf() {};

  //Description:
  //Get state from our renderable.
  virtual void SynchronizeSelf();

  //Description:
  //Override to interface to a specific backend.
  virtual void RenderSelf() {};

protected:
  vtkLightNode();
  ~vtkLightNode();

  //todo: use a map with string keys being renderable's member name
  //state
  double AmbientColor[3];
  double AttenuationValues[3];
  double ConeAngle;
  double DiffuseColor[3];
  double Exponent;
  double FocalPoint[3];
  double Intensity;
  int LightType;
  double Position[3];
  bool Positional;
  double SpecularColor[3];
  bool Switch;

private:
  vtkLightNode(const vtkLightNode&); // Not implemented.
  void operator=(const vtkLightNode&); // Not implemented.
};

#endif
