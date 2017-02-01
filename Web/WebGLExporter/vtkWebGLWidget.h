/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebGLWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWebGLWidget
 *
 * Widget representation for WebGL.
*/

#ifndef vtkWebGLWidget_h
#define vtkWebGLWidget_h

#include "vtkWebGLObject.h"
#include "vtkWebGLExporterModule.h" // needed for export macro

#include <vector> // Needed to store colors

class vtkActor2D;

class VTKWEBGLEXPORTER_EXPORT vtkWebGLWidget : public vtkWebGLObject
{
public:
  static vtkWebGLWidget* New();
  vtkTypeMacro(vtkWebGLWidget, vtkWebGLObject);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  void GenerateBinaryData() VTK_OVERRIDE;
  unsigned char* GetBinaryData(int part) VTK_OVERRIDE;
  int GetBinarySize(int part) VTK_OVERRIDE;
  int GetNumberOfParts() VTK_OVERRIDE;

  void GetDataFromColorMap(vtkActor2D* actor);

protected:
    vtkWebGLWidget();
    ~vtkWebGLWidget();

    unsigned char* binaryData;
    int binarySize;
    int orientation;
    char* title;
    char* textFormat;
    int textPosition;
    float position[2];
    float size[2];
    int numberOfLabels;
    std::vector <double*>colors;      //x, r, g, b

private:
  vtkWebGLWidget(const vtkWebGLWidget&) VTK_DELETE_FUNCTION;
  void operator=(const vtkWebGLWidget&) VTK_DELETE_FUNCTION;

};

#endif
