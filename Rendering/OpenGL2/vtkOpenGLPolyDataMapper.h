/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLPolyDataMapper - PolyDataMapper using OpenGL to render.
// .SECTION Description
// PolyDataMapper that uses a OpenGL to do the actual rendering.

#ifndef vtkOpenGLPolyDataMapper_h
#define vtkOpenGLPolyDataMapper_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkPolyDataMapper.h"
#include "vtkShader.h" // for methods
#include "vtkOpenGLHelper.h" // used for ivars
#include <vector> //for ivars
#include <map> //for methods

class vtkCellArray;
class vtkMatrix4x4;
class vtkMatrix3x3;
class vtkOpenGLTexture;
class vtkOpenGLBufferObject;
class vtkOpenGLVertexBufferObject;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLPolyDataMapper : public vtkPolyDataMapper
{
public:
  static vtkOpenGLPolyDataMapper* New();
  vtkTypeMacro(vtkOpenGLPolyDataMapper, vtkPolyDataMapper)
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implemented by sub classes. Actual rendering is done here.
  virtual void RenderPiece(vtkRenderer *ren, vtkActor *act);

  // Description:
  // Implemented by sub classes. Actual rendering is done here.
  virtual void RenderPieceStart(vtkRenderer *ren, vtkActor *act);
  virtual void RenderPieceDraw(vtkRenderer *ren, vtkActor *act);
  virtual void RenderPieceFinish(vtkRenderer *ren, vtkActor *act);
  virtual void RenderEdges(vtkRenderer *ren, vtkActor *act);

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  vtkGetMacro(PopulateSelectionSettings,int);
  void SetPopulateSelectionSettings(int v) { this->PopulateSelectionSettings = v; };

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Used by vtkHardwareSelector to determine if the prop supports hardware
  // selection.
  virtual bool GetSupportsSelection() { return true; }

  // Description:
  // Returns if the mapper does not expect to have translucent geometry. This
  // may happen when using ScalarMode is set to not map scalars i.e. render the
  // scalar array directly as colors and the scalar array has opacity i.e. alpha
  // component. Note that even if this method returns true, an actor may treat
  // the geometry as translucent since a constant translucency is set on the
  // property, for example.
  // Overridden to use the actual data and ScalarMode to determine if we have
  // opaque geometry.
  virtual bool GetIsOpaque();

  // used by RenderPiece and functions it calls to reduce
  // calls to get the input and allow for rendering of
  // other polydata (not the input)
  vtkPolyData *CurrentInput;

  // Description:
  // By default, this class uses the dataset's point and cell ids during
  // rendering. However, one can override those by specifying cell and point
  // data arrays to use instead. Currently, only vtkIdType array is supported.
  // Set to NULL string (default) to use the point ids instead.
  vtkSetStringMacro(PointIdArrayName);
  vtkGetStringMacro(PointIdArrayName);
  vtkSetStringMacro(CellIdArrayName);
  vtkGetStringMacro(CellIdArrayName);

  // Description:
  // If this class should override the process id using a data-array,
  // set this variable to the name of the array to use. It must be a
  // point-array.
  vtkSetStringMacro(ProcessIdArrayName);
  vtkGetStringMacro(ProcessIdArrayName);

  // Description:
  // Generally, this class can render the composite id when iterating
  // over composite datasets. However in some cases (as in AMR), the rendered
  // structure may not correspond to the input data, in which case we need
  // to provide a cell array that can be used to render in the composite id in
  // selection passes. Set to NULL (default) to not override the composite id
  // color set by vtkCompositePainter if any.
  // The array *MUST* be a cell array and of type vtkUnsignedIntArray.
  vtkSetStringMacro(CompositeIdArrayName);
  vtkGetStringMacro(CompositeIdArrayName);


  // Description:
  // This function enables you to apply your own substitutions
  // to the shader creation process. The shader code in this class
  // is created by applying a bunch of string replacements to a
  // shader template. Using this function you can apply your
  // own string replacements to add features you desire.
  void AddShaderReplacement(
    vtkShader::Type shaderType, // vertex, fragment, etc
    std::string originalValue,
    bool replaceFirst,  // do this replacement before the default
    std::string replacementValue,
    bool replaceAll);
  void ClearShaderReplacement(
    vtkShader::Type shaderType, // vertex, fragment, etc
    std::string originalValue,
    bool replaceFirst);

  // Description:
  // Allow the program to set the shader codes used directly
  // instead of using the built in templates. Be aware, if
  // set, this template will be used for all cases,
  // primitive types, picking etc.
  vtkSetStringMacro(VertexShaderCode);
  vtkGetStringMacro(VertexShaderCode);
  vtkSetStringMacro(FragmentShaderCode);
  vtkGetStringMacro(FragmentShaderCode);
  vtkSetStringMacro(GeometryShaderCode);
  vtkGetStringMacro(GeometryShaderCode);

  // the following is all extra stuff to work around the
  // fact that gl_PrimitiveID does not work correctly on
  // Apple devices with AMD graphics hardware. See apple
  // bug ID 20747550
  static vtkPolyData *HandleAppleBug(
    vtkPolyData *poly,
    std::vector<float> &buffData);

  // Description:
  // Make a shallow copy of this mapper.
  void ShallowCopy(vtkAbstractMapper *m);

  // Description:
  // Override the normal test for the apple bug
  void ForceHaveAppleBugOff()
  {
    this->HaveAppleBugForce = 1;
    this->Modified();
  }
  void ForceHaveAppleBugOn()
  {
    this->HaveAppleBugForce = 2;
    this->Modified();
  }

  // Description:
  // Get the value of HaveAppleBug
  bool GetHaveAppleBug() { return this->HaveAppleBug; }

protected:
  vtkOpenGLPolyDataMapper();
  ~vtkOpenGLPolyDataMapper();

  // the following is all extra stuff to work around the
  // fact that gl_PrimitiveID does not work correctly on
  // Apple devices with AMD graphics hardware. See apple
  // bug ID 20747550
  bool HaveAppleBug;
  int HaveAppleBugForce; // 0 = default 1 = 0ff 2 = on
  std::vector<float> AppleBugPrimIDs;
  vtkOpenGLBufferObject *AppleBugPrimIDBuffer;

  // Description:
  // helper function to get the appropriate coincident params
  void GetCoincidentParameters(vtkActor *actor, float &factor, float &offset);

  // Description:
  // Called in GetBounds(). When this method is called, the consider the input
  // to be updated depending on whether this->Static is set or not. This method
  // simply obtains the bounds from the data-object and returns it.
  virtual void ComputeBounds();

  // Description:
  // Make sure appropriate shaders are defined, compiled and bound.  This method
  // orchistrates the process, much of the work is done in other methods
  virtual void UpdateShaders(
    vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkActor *act);

  // Description:
  // Does the shader source need to be recomputed
  virtual bool GetNeedToRebuildShaders(
    vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkActor *act);

  // Description:
  // Build the shader source code, called by UpdateShader
  virtual void BuildShaders(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act);

  // Description:
  // Create the basic shaders before replacement
  virtual void GetShaderTemplate(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act);

  // Description:
  // Perform string replacments on the shader templates
  virtual void ReplaceShaderValues(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act);

  // Description:
  // Perform string replacments on the shader templates, called from
  // ReplaceShaderValues
  virtual void ReplaceShaderColor(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act);
  virtual void ReplaceShaderLight(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act);
  virtual void ReplaceShaderTCoord(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act);
  virtual void ReplaceShaderPicking(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act);
  virtual void ReplaceShaderDepthPeeling(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act);
  virtual void ReplaceShaderPrimID(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act);
  virtual void ReplaceShaderNormal(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act);
  virtual void ReplaceShaderClip(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act);
  virtual void ReplaceShaderPositionVC(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act);
  virtual void ReplaceShaderCoincidentOffset(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act);

  // Description:
  // Set the shader parameters related to the mapper/input data, called by UpdateShader
  virtual void SetMapperShaderParameters(vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkActor *act);

  // Description:
  // Set the shader parameteres related to lighting, called by UpdateShader
  virtual void SetLightingShaderParameters(vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkActor *act);

  // Description:
  // Set the shader parameteres related to the Camera, called by UpdateShader
  virtual void SetCameraShaderParameters(vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkActor *act);

  // Description:
  // Set the shader parameteres related to the property, called by UpdateShader
  virtual void SetPropertyShaderParameters(vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkActor *act);

  // Description:
  // Update the VBO/IBO to be current
  virtual void UpdateBufferObjects(vtkRenderer *ren, vtkActor *act);

  // Description:
  // Does the VBO/IBO need to be rebuilt
  virtual bool GetNeedToRebuildBufferObjects(vtkRenderer *ren, vtkActor *act);

  // Description:
  // Build the VBO/IBO, called by UpdateBufferObjects
  virtual void BuildBufferObjects(vtkRenderer *ren, vtkActor *act);

  // Description:
  // Build the IBO, called by BuildBufferObjects
  virtual void BuildIBO(vtkRenderer *ren, vtkActor *act, vtkPolyData *poly);

  // The VBO and its layout.
  vtkOpenGLVertexBufferObject *VBO;

  // Structures for the various cell types we render.
  vtkOpenGLHelper Points;
  vtkOpenGLHelper Lines;
  vtkOpenGLHelper Tris;
  vtkOpenGLHelper TriStrips;
  vtkOpenGLHelper TrisEdges;
  vtkOpenGLHelper TriStripsEdges;
  vtkOpenGLHelper *LastBoundBO;
  bool DrawingEdges;

  // do we have wide lines that require special handling
  virtual bool HaveWideLines(vtkRenderer *, vtkActor *);

  // values we use to determine if we need to rebuild shaders
  std::map<const vtkOpenGLHelper *, int> LastLightComplexity;
  std::map<const vtkOpenGLHelper *, vtkTimeStamp> LightComplexityChanged;

  int LastSelectionState;
  vtkTimeStamp SelectionStateChanged;

  int LastDepthPeeling;
  vtkTimeStamp DepthPeelingChanged;

  bool UsingScalarColoring;
  vtkTimeStamp VBOBuildTime; // When was the OpenGL VBO updated?
  std::string VBOBuildString; // used for determining whento rebuild the VBO
  std::string IBOBuildString; // used for determining whento rebuild the IBOs
  vtkOpenGLTexture* InternalColorTexture;

  int PopulateSelectionSettings;
  int PrimitiveIDOffset;

  vtkMatrix4x4 *TempMatrix4;
  vtkMatrix3x3 *TempMatrix3;

  // if set to true, tcoords will be passed to the
  // VBO even if the mapper knows of no texture maps
  // normally tcoords are only added to the VBO if the
  // mapper has indentified a texture map as well.
  bool ForceTextureCoordinates;

  void BuildCellTextures(
    vtkRenderer *ren,
    vtkActor *,
    vtkCellArray *prims[4],
    int representation);

  void AppendCellTextures(
    vtkRenderer *ren,
    vtkActor *,
    vtkCellArray *prims[4],
    int representation,
    std::vector<unsigned char> &colors,
    std::vector<float> &normals,
    vtkPolyData *pd);

  bool HavePickScalars;
  vtkTextureObject *CellScalarTexture;
  vtkOpenGLBufferObject *CellScalarBuffer;
  bool HaveCellScalars;
  vtkTextureObject *CellNormalTexture;
  vtkOpenGLBufferObject *CellNormalBuffer;
  bool HaveCellNormals;

  // aditional picking indirection
  char* PointIdArrayName;
  char* CellIdArrayName;
  char* ProcessIdArrayName;
  char* CompositeIdArrayName;

  int TextureComponents;

  class ReplacementSpec
    {
    public:
      std::string OriginalValue;
      vtkShader::Type ShaderType;
      bool ReplaceFirst;
      bool operator<(const ReplacementSpec &v1) const
        {
        if (this->OriginalValue != v1.OriginalValue) { return this->OriginalValue < v1.OriginalValue; }
        if (this->ShaderType != v1.ShaderType) { return this->ShaderType < v1.ShaderType; }
        return (this->ReplaceFirst < v1.ReplaceFirst);
        }
      bool operator>(const ReplacementSpec &v1) const
        {
        if (this->OriginalValue != v1.OriginalValue) { return this->OriginalValue > v1.OriginalValue; }
        if (this->ShaderType != v1.ShaderType) { return this->ShaderType > v1.ShaderType; }
        return (this->ReplaceFirst > v1.ReplaceFirst);
        }
      };
  class ReplacementValue
    {
    public:
      std::string Replacement;
      bool ReplaceAll;
    };

  std::map<const ReplacementSpec,ReplacementValue> UserShaderReplacements;

  char *VertexShaderCode;
  char *FragmentShaderCode;
  char *GeometryShaderCode;

private:
  vtkOpenGLPolyDataMapper(const vtkOpenGLPolyDataMapper&); // Not implemented.
  void operator=(const vtkOpenGLPolyDataMapper&); // Not implemented.
};

#endif
