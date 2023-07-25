// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGLWidget
 * @brief   Widget representation for WebGL.
 */

#ifndef vtkWebGLWidget_h
#define vtkWebGLWidget_h

#include "vtkWebGLExporterModule.h" // needed for export macro
#include "vtkWebGLObject.h"

#include <vector> // Needed to store colors

VTK_ABI_NAMESPACE_BEGIN
class vtkActor2D;

class VTKWEBGLEXPORTER_EXPORT vtkWebGLWidget : public vtkWebGLObject
{
public:
  static vtkWebGLWidget* New();
  vtkTypeMacro(vtkWebGLWidget, vtkWebGLObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void GenerateBinaryData() override;
  unsigned char* GetBinaryData(int part) override;
  int GetBinarySize(int part) override;
  int GetNumberOfParts() override;

  void GetDataFromColorMap(vtkActor2D* actor);

protected:
  vtkWebGLWidget();
  ~vtkWebGLWidget() override;

  unsigned char* binaryData;
  int binarySize;
  int orientation;
  char* title;
  char* textFormat;
  int textPosition;
  float position[2];
  float size[2];
  int numberOfLabels;
  std::vector<double*> colors; // x, r, g, b

private:
  vtkWebGLWidget(const vtkWebGLWidget&) = delete;
  void operator=(const vtkWebGLWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
