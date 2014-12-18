/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtTreeRingLabelMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkQtTreeRingLabelMapper - draw text labels on a tree map
//
// .SECTION Description
// vtkQtTreeRingLabelMapper is a mapper that renders text on a tree map.
// A tree map is a vtkTree with an associated 4-tuple array
// used for storing the boundary rectangle for each vertex in the tree.
// The user must specify the array name used for storing the rectangles.
//
// The mapper iterates through the tree and attempts and renders a label
// inside the vertex's rectangle as long as the following conditions hold:
// 1. The vertex level is within the range of levels specified for labeling.
// 2. The label can fully fit inside its box.
// 3. The label does not overlap an ancestor's label.
//
// .SECTION See Also
// vtkLabeledDataMapper
//
// .SECTION Thanks
// Thanks to Jason Shepherd from
// Sandia National Laboratories for help in developing this class.

#ifndef vtkQtTreeRingLabelMapper_h
#define vtkQtTreeRingLabelMapper_h

#include "vtkRenderingQtModule.h" // For export macro
#include "vtkLabeledDataMapper.h"

//BTX
class QImage;
//ETX

class vtkQImageToImageSource;
class vtkCoordinate;
class vtkDoubleArray;
class vtkPlaneSource;
class vtkPolyDataMapper2D;
class vtkRenderer;
class vtkStringArray;
class vtkTexture;
class vtkTextureMapToPlane;
class vtkTree;
class vtkUnicodeStringArray;

class VTKRENDERINGQT_EXPORT vtkQtTreeRingLabelMapper : public vtkLabeledDataMapper
{
public:
  static vtkQtTreeRingLabelMapper *New();
  vtkTypeMacro(vtkQtTreeRingLabelMapper,vtkLabeledDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Draw the text to the screen at each input point.
  virtual void RenderOpaqueGeometry(vtkViewport* viewport, vtkActor2D* actor);
  virtual void RenderOverlay(vtkViewport *viewport, vtkActor2D *actor);

  // Description:
  // The input to this filter.
  virtual vtkTree *GetInputTree();

  // Description:
  // The name of the 4-tuple array used for
  virtual void SetSectorsArrayName(const char* name);

  // Description:
  // Set/Get the text property. Note that multiple type text properties
  // (set with a second integer parameter) are not currently supported,
  // but are provided to avoid compiler warnings.
  virtual void SetLabelTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(LabelTextProperty,vtkTextProperty);
  virtual void SetLabelTextProperty(vtkTextProperty *p, int type)
    { this->Superclass::SetLabelTextProperty(p, type); }
  virtual vtkTextProperty* GetLabelTextProperty(int type)
    { return this->Superclass::GetLabelTextProperty(type); }

  // Description:
  // Set/Get the name of the text rotation array.
  vtkSetStringMacro(TextRotationArrayName);
  vtkGetStringMacro(TextRotationArrayName);

  // Description:
  // Return the object's MTime. This is overridden to include
  // the timestamp of its internal class.
  virtual unsigned long GetMTime();

  void SetRenderer(vtkRenderer* ren)
    {
      if (this->Renderer != ren)
        {
        this->Renderer = ren;
        this->Modified();
        }
    }
  vtkRenderer* GetRenderer() { return this->Renderer; }

protected:
  vtkQtTreeRingLabelMapper();
  ~vtkQtTreeRingLabelMapper();
  void LabelTree(vtkTree *tree, vtkDataArray *sectorInfo,
                 vtkDataArray *numericData, vtkStringArray *stringData, vtkUnicodeStringArray *uStringData,
                 int activeComp, int numComps, vtkViewport* viewport);
  void GetVertexLabel(vtkIdType vertex, vtkDataArray *numericData,
                      vtkStringArray *stringData,
                      vtkUnicodeStringArray *uStringData,
                      int activeComp, int numComps,
                      char *string);

  //Returns true if the center of the sector is in the window
  // along with the pixel dimensions (width, height)  of the sector
  bool PointInWindow(double *sinfo, double *newDim, double *textPosDC, vtkViewport *viewport);

  vtkViewport *CurrentViewPort;
  vtkCoordinate *VCoord;
  vtkQImageToImageSource* QtImageSource;
  vtkPlaneSource* PlaneSource;
  vtkRenderer* Renderer;
  vtkTextProperty *LabelTextProperty;
  vtkTexture* LabelTexture;
  vtkTextureMapToPlane* TextureMapToPlane;
  char* TextRotationArrayName;
  vtkPolyDataMapper2D* polyDataMapper;
  QImage* QtImage;
  int WindowSize[2];

private:
  vtkQtTreeRingLabelMapper(const vtkQtTreeRingLabelMapper&);  // Not implemented.
  void operator=(const vtkQtTreeRingLabelMapper&);  // Not implemented.
};


#endif
