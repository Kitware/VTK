/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositePolyDataMapper2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompositePolyDataMapper2 - mapper for composite dataset
// .SECTION Description
// vtkCompositePolyDataMapper2 is similar to
// vtkGenericCompositePolyDataMapper2 but requires that its inputs all have the
// same properties (normals, tcoord, scalars, etc) It will only draw
// polys and it does not support edge flags. The advantage to using
// this class is that it generally should be faster

#ifndef vtkCompositePolyDataMapper2_h
#define vtkCompositePolyDataMapper2_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkGenericCompositePolyDataMapper2.h"

class VTKRENDERINGOPENGL2_EXPORT vtkCompositePolyDataMapper2 : public vtkGenericCompositePolyDataMapper2
{
public:
  static vtkCompositePolyDataMapper2* New();
  vtkTypeMacro(vtkCompositePolyDataMapper2, vtkGenericCompositePolyDataMapper2);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This calls RenderPiece (in a for loop if streaming is necessary).
  virtual void Render(vtkRenderer *ren, vtkActor *act);

  virtual void RenderPiece(vtkRenderer *ren, vtkActor *act);
  virtual void RenderPieceDraw(vtkRenderer *ren, vtkActor *act);
  virtual void RenderEdges(vtkRenderer *ren, vtkActor *act);

protected:
  vtkCompositePolyDataMapper2();
  ~vtkCompositePolyDataMapper2();

  // Description:
  // Perform string replacments on the shader templates, called from
  // ReplaceShaderValues
  virtual void ReplaceShaderColor(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act);

  // Description:
  // Build the VBO/IBO, called by UpdateBufferObjects
  virtual void BuildBufferObjects(vtkRenderer *ren, vtkActor *act);
  virtual void AppendOneBufferObject(vtkRenderer *ren,
    vtkActor *act, vtkPolyData *pd, unsigned int flat_index,
    std::vector<unsigned char> &colors,
    std::vector<float> &norms);

  std::vector<unsigned int> VertexOffsets;
  std::vector<unsigned int> IndexOffsets;
  std::vector<unsigned int> IndexArray;
  std::vector<unsigned int> EdgeIndexArray;
  std::vector<unsigned int> EdgeIndexOffsets;
  unsigned int MaximumFlatIndex;

  class RenderValue
    {
    public:
      unsigned int StartVertex;
      unsigned int StartIndex;
      unsigned int StartEdgeIndex;
      unsigned int EndVertex;
      unsigned int EndIndex;
      unsigned int EndEdgeIndex;
      double Opacity;
      bool OverridesColor;
      bool Visibility;
      vtkColor3d Color;
      unsigned int PickId;
    };

  std::vector<RenderValue> RenderValues;
  vtkTimeStamp RenderValuesBuildTime;

  bool UseGeneric;  // use the generic render
  vtkTimeStamp GenericTestTime;

  // free up memory
  void FreeStructures();

  void BuildRenderValues(vtkRenderer *renderer,
    vtkActor *actor,
    vtkDataObject *dobj,
    unsigned int &flat_index,
    unsigned int &lastVertex,
    unsigned int &lastIndex,
    unsigned int &lastEdgeIndex);

  // Description:
  // Returns if we can use texture maps for scalar coloring. Note this doesn't
  // say we "will" use scalar coloring. It says, if we do use scalar coloring,
  // we will use a texture.
  // When rendering multiblock datasets, if any 2 blocks provide different
  // lookup tables for the scalars, then also we cannot use textures. This case
  // can be handled if required.
  virtual int CanUseTextureMapForColoring(vtkDataObject* input);
  bool CanUseTextureMapForColoringSet;
  int CanUseTextureMapForColoringValue;

private:
  vtkCompositePolyDataMapper2(
    const vtkCompositePolyDataMapper2&); // Not implemented.
  void operator=(const vtkCompositePolyDataMapper2&); // Not implemented.
};

#endif
