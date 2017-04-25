/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebGLObject.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWebGLObject
 *
 * vtkWebGLObject represent and manipulate an WebGL object and its data.
*/

#ifndef vtkWebGLObject_h
#define vtkWebGLObject_h

#include "vtkObject.h"
#include "vtkWebGLExporterModule.h" // needed for export macro

#include <string> // needed for ID and md5 storing

class vtkMatrix4x4;
class vtkUnsignedCharArray;

enum WebGLObjectTypes {
  wPOINTS = 0,
  wLINES = 1,
  wTRIANGLES = 2
};

class VTKWEBGLEXPORTER_EXPORT vtkWebGLObject : public vtkObject
{
public:
  static vtkWebGLObject* New();
  vtkTypeMacro(vtkWebGLObject, vtkObject)
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  virtual void GenerateBinaryData();
  virtual unsigned char* GetBinaryData(int part);
  virtual int GetBinarySize(int part);
  virtual int GetNumberOfParts();

  /**
   * This is a wrapper friendly method for access the binary data.
   * The binary data for the requested part will be copied into the
   * given vtkUnsignedCharArray.
   */
  void GetBinaryData(int part, vtkUnsignedCharArray* buffer);

  void SetLayer(int l);
  void SetRendererId(size_t i);
  void SetId(const std::string& i);
  void SetWireframeMode(bool wireframe);
  void SetVisibility(bool vis);
  void SetTransformationMatrix(vtkMatrix4x4* m);
  void SetIsWidget(bool w);
  void SetHasTransparency(bool t);
  void SetInteractAtServer(bool i);
  void SetType(WebGLObjectTypes t);
  bool isWireframeMode();
  bool isVisible();
  bool HasChanged();
  bool isWidget();
  bool HasTransparency();
  bool InteractAtServer();

  std::string GetMD5();
  std::string GetId();

  size_t GetRendererId();
  int GetLayer();

protected:
    vtkWebGLObject();
    ~vtkWebGLObject();

    float Matrix[16];
    size_t rendererId;
    int layer;                  // Renderer Layer
    std::string id;          // Id of the object
    std::string MD5;
    bool hasChanged;
    bool iswireframeMode;
    bool isvisible;
    WebGLObjectTypes webGlType;
    bool hasTransparency;
    bool iswidget;
    bool interactAtServer;

private:
  vtkWebGLObject(const vtkWebGLObject&) VTK_DELETE_FUNCTION;
  void operator=(const vtkWebGLObject&) VTK_DELETE_FUNCTION;
};

#endif
