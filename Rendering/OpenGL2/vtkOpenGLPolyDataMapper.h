// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLPolyDataMapper
 * @brief   PolyDataMapper using OpenGL to render.
 *
 * PolyDataMapper that uses a OpenGL to do the actual rendering.
 */

#ifndef vtkOpenGLPolyDataMapper_h
#define vtkOpenGLPolyDataMapper_h

#include "vtkInformation.h"  // for prim struct
#include "vtkNew.h"          // For vtkNew
#include "vtkOpenGLHelper.h" // used for ivars
#include "vtkPolyDataMapper.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkShader.h"                 // for methods
#include "vtkStateStorage.h"           // used for ivars
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

#include <map>    // for map
#include <tuple>  // for tuple
#include <vector> // for vector

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkGenericOpenGLResourceFreeCallback;
class vtkMatrix4x4;
class vtkMatrix3x3;
class vtkOpenGLCellToVTKCellMap;
class vtkOpenGLRenderTimer;
class vtkOpenGLTexture;
class vtkOpenGLBufferObject;
class vtkOpenGLVertexBufferObject;
class vtkOpenGLVertexBufferObjectGroup;
class vtkPoints;
class vtkTexture;
class vtkTextureObject;
class vtkTransform;
class vtkOpenGLShaderProperty;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpenGLPolyDataMapper : public vtkPolyDataMapper
{
public:
  static vtkOpenGLPolyDataMapper* New();
  vtkTypeMacro(vtkOpenGLPolyDataMapper, vtkPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Implemented by sub classes. Actual rendering is done here.
   */
  void RenderPiece(vtkRenderer* ren, vtkActor* act) override;

  /**
   * Unique hash based on availability of scalars, normals, tcoords, lookup tables
   * and related attributes that distinguish the rendering requirements of different
   * polydata.
   */
  MapperHashType GenerateHash(vtkPolyData* polydata) override;

  ///@{
  /**
   * Implemented by sub classes. Actual rendering is done here.
   */
  virtual void RenderPieceStart(vtkRenderer* ren, vtkActor* act);
  virtual void RenderPieceDraw(vtkRenderer* ren, vtkActor* act);
  virtual void RenderPieceFinish(vtkRenderer* ren, vtkActor* act);
  ///@}

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  vtkGetMacro(PopulateSelectionSettings, int);
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  void SetPopulateSelectionSettings(int v) { this->PopulateSelectionSettings = v; }

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Used by vtkHardwareSelector to determine if the prop supports hardware
   * selection.
   */
  bool GetSupportsSelection() override { return true; }

  // used by RenderPiece and functions it calls to reduce
  // calls to get the input and allow for rendering of
  // other polydata (not the input)
  vtkPolyData* CurrentInput;

  /**
   * Make a shallow copy of this mapper.
   */
  void ShallowCopy(vtkAbstractMapper* m) override;

  /// Return the mapper's vertex buffer objects.
  vtkGetObjectMacro(VBOs, vtkOpenGLVertexBufferObjectGroup);

  /**\brief A convenience method for enabling/disabling
   *   the VBO's shift+scale transform.
   */
  void SetVBOShiftScaleMethod(int method) override;

  /**
   * Allow the shader code to set the point size (with gl_PointSize variable)
   * instead of using the one defined by the property. Note that this flag is
   * not available on OpenGLES as the feature is enabled by default. With
   * OpenGL, the feature is turned off by default.
   * Warning: on MacOS, enabling the feature result in non point drawing
   * if the shaders do not set the point size.
   */
  vtkGetMacro(UseProgramPointSize, bool);
  vtkSetMacro(UseProgramPointSize, bool);
  vtkBooleanMacro(UseProgramPointSize, bool);

  enum PrimitiveTypes
  {
    PrimitiveStart = 0,
    PrimitivePoints = 0,
    PrimitiveLines,
    PrimitiveTris,
    PrimitiveTriStrips,
    PrimitiveVertices,
    PrimitiveEnd
  };

  /**
   * Select a data array from the point/cell data
   * and map it to a generic vertex attribute.
   * vertexAttributeName is the name of the vertex attribute.
   * dataArrayName is the name of the data array.
   * fieldAssociation indicates when the data array is a point data array or
   * cell data array (vtkDataObject::FIELD_ASSOCIATION_POINTS or
   * (vtkDataObject::FIELD_ASSOCIATION_CELLS).
   * componentno indicates which component from the data array must be passed as
   * the attribute. If -1, then all components are passed.
   */
  void MapDataArrayToVertexAttribute(const char* vertexAttributeName, const char* dataArrayName,
    int fieldAssociation, int componentno = -1) override;

  // This method will Map the specified data array for use as
  // a texture coordinate for texture tname. The actual
  // attribute will be named tname_coord so as to not
  // conflict with the texture sampler definition which will
  // be tname.
  void MapDataArrayToMultiTextureAttribute(const char* tname, const char* dataArrayName,
    int fieldAssociation, int componentno = -1) override;

  /**
   * Remove a vertex attribute mapping.
   */
  void RemoveVertexAttributeMapping(const char* vertexAttributeName) override;

  /**
   * Remove all vertex attributes.
   */
  void RemoveAllVertexAttributeMappings() override;

  /**
   * allows a mapper to update a selections color buffers
   * Called from a prop which in turn is called from the selector
   */
  void ProcessSelectorPixelBuffers(
    vtkHardwareSelector* sel, std::vector<unsigned int>& pixeloffsets, vtkProp* prop) override;

protected:
  vtkOpenGLPolyDataMapper();
  ~vtkOpenGLPolyDataMapper() override;

  vtkGenericOpenGLResourceFreeCallback* ResourceCallback;

  void MapDataArray(const char* vertexAttributeName, const char* dataArrayName,
    const char* texturename, int fieldAssociation, int componentno);

  // what coordinate should be used for this texture
  std::string GetTextureCoordinateName(const char* tname);

  // handle updating shift scale based on pose changes
  virtual void UpdateCameraShiftScale(vtkRenderer* ren, vtkActor* actor);

  /**
   * helper function to get the appropriate coincident params
   */
  void GetCoincidentParameters(vtkRenderer* ren, vtkActor* actor, float& factor, float& offset);

  /**
   * Called in GetBounds(). When this method is called, the consider the input
   * to be updated depending on whether this->Static is set or not. This method
   * simply obtains the bounds from the data-object and returns it.
   */
  void ComputeBounds() override;

  /**
   * Make sure appropriate shaders are defined, compiled and bound.  This method
   * orchistrates the process, much of the work is done in other methods
   */
  virtual void UpdateShaders(vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* act);

  /**
   * Does the shader source need to be recomputed
   */
  virtual bool GetNeedToRebuildShaders(vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* act);

  /**
   * Build the shader source code, called by UpdateShader
   */
  virtual void BuildShaders(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act);

  /**
   * Create the basic shaders before replacement
   */
  virtual void GetShaderTemplate(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act);

  /**
   * Perform string replacements on the shader templates
   */
  virtual void ReplaceShaderValues(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act);

  ///@{
  /**
   * Perform string replacements on the shader templates, called from
   * ReplaceShaderValues
   */
  virtual void ReplaceShaderRenderPass(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act, bool prePass);
  virtual void ReplaceShaderCustomUniforms(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkActor* act);
  virtual void ReplaceShaderColor(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act);
  virtual void ReplaceShaderEdges(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act);
  virtual void ReplaceShaderLight(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act);
  virtual void ReplaceShaderTCoord(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act);
  virtual void ReplaceShaderPicking(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act);
  virtual void ReplaceShaderPrimID(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act);
  virtual void ReplaceShaderNormal(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act);
  virtual void ReplaceShaderClip(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act);
  virtual void ReplaceShaderPositionVC(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act);
  virtual void ReplaceShaderCoincidentOffset(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act);
  virtual void ReplaceShaderDepth(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act);
  ///@}

  /**
   * Set the value of user-defined uniform variables, called by UpdateShader
   */
  virtual void SetCustomUniforms(vtkOpenGLHelper& cellBO, vtkActor* actor);

  /**
   * Set the shader parameters related to the mapper/input data, called by UpdateShader
   */
  virtual void SetMapperShaderParameters(vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* act);

  /**
   * Set the shader parameters related to lighting, called by UpdateShader
   */
  virtual void SetLightingShaderParameters(
    vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* act);

  /**
   * Set the shader parameters related to the Camera, called by UpdateShader
   */
  virtual void SetCameraShaderParameters(vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* act);

  /**
   * Set the shader parameters related to the property, called by UpdateShader
   */
  virtual void SetPropertyShaderParameters(
    vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* act);

  /**
   * Update the VBO/IBO to be current
   */
  virtual void UpdateBufferObjects(vtkRenderer* ren, vtkActor* act);

  /**
   * Does the VBO/IBO need to be rebuilt
   */
  virtual bool GetNeedToRebuildBufferObjects(vtkRenderer* ren, vtkActor* act);

  /**
   * Build the VBO/IBO, called by UpdateBufferObjects
   */
  virtual void BuildBufferObjects(vtkRenderer* ren, vtkActor* act);

  /**
   * Build the IBO, called by BuildBufferObjects
   */
  virtual void BuildIBO(vtkRenderer* ren, vtkActor* act, vtkPolyData* poly);

  /**
   * Build the selection IBO, called by UpdateBufferObjects
   */
  virtual void BuildSelectionIBO(
    vtkPolyData* poly, std::vector<unsigned int> (&indices)[4], vtkIdType offset);

  /**
   * Build the selection cache, used to map value ids to indices values
   */
  virtual void BuildSelectionCache(const char* arrayName, bool selectingPoints, vtkPolyData* poly);

  // The VBO and its layout.
  vtkOpenGLVertexBufferObjectGroup* VBOs;

  // Structures for the various cell types we render.
  vtkOpenGLHelper Primitives[PrimitiveEnd];
  vtkOpenGLHelper SelectionPrimitives[PrimitiveEnd];
  vtkOpenGLHelper* LastBoundBO;
  bool DrawingVertices;
  bool DrawingSelection = false;
  int SelectionType;
  vtkMTimeType SelectionTime = 0;

  std::map<std::tuple<unsigned int, unsigned int, vtkIdType>, std::vector<vtkIdType>>
    SelectionCache;
  std::string SelectionCacheName;
  bool SelectionCacheForPoints = false;
  vtkMTimeType SelectionCacheTime = 0;
  vtkPolyData* SelectionPolyData = nullptr;

  // do we have wide lines that require special handling
  virtual bool HaveWideLines(vtkRenderer*, vtkActor*);

  // do we have textures that require special handling
  virtual bool HaveTextures(vtkActor* actor);

  // how many textures do we have
  virtual unsigned int GetNumberOfTextures(vtkActor* actor);

  // populate a vector with the textures we have
  // the order is always
  //  ColorInternalTexture
  //  Actors texture
  //  Properties textures
  typedef std::pair<vtkTexture*, std::string> texinfo;
  virtual std::vector<texinfo> GetTextures(vtkActor* actor);

  // do we have textures coordinates that require special handling
  virtual bool HaveTCoords(vtkPolyData* poly);

  // values we use to determine if we need to rebuild shaders
  // stored in a map keyed on the vtkOpenGLHelper, so one
  // typically entry per type of primitive we render which
  // matches the shader programs we use
  class primitiveInfo
  {
  public:
    /**
     * Represent the type of lighting used.
     *
     * Forwarded from vtkOpenGLRenderer::LightingComplexityEnum.
     */
    enum LightingTypeEnum
    {
      NoLighting = 0,
      Headlight = 1,
      Directional = 2,
      Positional = 3
    };
    LightingTypeEnum LastLightComplexity = NoLighting;

    int LastLightCount;

    vtkTimeStamp LightComplexityChanged;

    // Caches the vtkOpenGLRenderPass::RenderPasses() information.
    // Note: Do not dereference the pointers held by this object. There is no
    // guarantee that they are still valid!
    vtkNew<vtkInformation> LastRenderPassInfo;
  };
  std::map<const vtkOpenGLHelper*, primitiveInfo> PrimitiveInfo;

  bool PointPicking;
  int LastSelectionState;
  vtkTimeStamp SelectionStateChanged;

  // Check the renderpasses in actor's property keys to see if they've changed
  // render stages:
  vtkMTimeType GetRenderPassStageMTime(vtkActor* actor, const vtkOpenGLHelper* cellBO);

  bool UsingScalarColoring;
  vtkTimeStamp VBOBuildTime;     // When was the OpenGL VBO updated?
  vtkStateStorage VBOBuildState; // used for determining when to rebuild the VBO
  vtkStateStorage IBOBuildState; // used for determining whento rebuild the IBOs
  vtkStateStorage CellTextureBuildState;
  vtkStateStorage TempState; // can be used to avoid constant allocs/deallocs
  vtkOpenGLTexture* InternalColorTexture;

  int PopulateSelectionSettings;
  vtkIdType PrimitiveIDOffset;

  vtkMatrix4x4* TempMatrix4;
  vtkMatrix3x3* TempMatrix3;
  vtkNew<vtkTransform> VBOInverseTransform;
  vtkNew<vtkMatrix4x4> VBOShiftScale;
  bool UseProgramPointSize;

  // if set to true, tcoords will be passed to the
  // VBO even if the mapper knows of no texture maps
  // normally tcoords are only added to the VBO if the
  // mapper has identified a texture map as well.
  bool ForceTextureCoordinates;

  virtual void BuildCellTextures(
    vtkRenderer* ren, vtkActor*, vtkCellArray* prims[4], int representation);

  void AppendCellTextures(vtkRenderer* ren, vtkActor*, vtkCellArray* prims[4], int representation,
    std::vector<unsigned char>& colors, std::vector<float>& normals, vtkPolyData* pd,
    vtkOpenGLCellToVTKCellMap* ccmap);

  vtkTextureObject* CellScalarTexture;
  vtkOpenGLBufferObject* CellScalarBuffer;
  bool HaveCellScalars;
  vtkTextureObject* CellNormalTexture;
  vtkOpenGLBufferObject* CellNormalBuffer;
  bool HaveCellNormals;

  vtkTextureObject* EdgeTexture;
  vtkOpenGLBufferObject* EdgeBuffer;
  std::vector<unsigned char> EdgeValues;
  virtual bool DrawingEdges(vtkRenderer*, vtkActor*);

  class ExtraAttributeValue
  {
  public:
    std::string DataArrayName;
    int FieldAssociation;
    int ComponentNumber;
    std::string TextureName;
  };
  std::map<std::string, ExtraAttributeValue> ExtraAttributes;

  vtkOpenGLRenderTimer* TimerQuery;

  // are we currently drawing spheres/tubes
  bool DrawingSpheres(vtkOpenGLHelper& cellBO, vtkActor* actor);
  bool DrawingTubes(vtkOpenGLHelper& cellBO, vtkActor* actor);
  bool DrawingTubesOrSpheres(vtkOpenGLHelper& cellBO, vtkActor* actor);

  // get which opengl mode to use to draw the primitive
  int GetOpenGLMode(int representation, int primType);

  // get how big to make the points when doing point picking
  // typically 2 for points, 4 for lines, 6 for surface
  int GetPointPickingPrimitiveSize(int primType);

  // used to occasionally invoke timers
  unsigned int TimerQueryCounter;

  // stores the mapping from OpenGL primitives IDs (gl_PrimitiveId) to VTK cells IDs
  vtkNew<vtkOpenGLCellToVTKCellMap> CellCellMap;

  // compute and set the maximum point and cell ID used in selection
  virtual void UpdateMaximumPointCellIds(vtkRenderer* ren, vtkActor* actor);

  virtual void AddPointIdsToSelectionPrimitives(vtkPolyData* poly, const char* arrayName,
    unsigned int processId, unsigned int compositeIndex, vtkIdType selectedId);
  virtual void AddCellIdsToSelectionPrimitives(vtkPolyData* poly, const char* arrayName,
    unsigned int processId, unsigned int compositeIndex, vtkIdType selectedId);

  vtkNew<vtkCellArray> SelectionArrays[4];

  vtkMTimeType EnvironmentTextureTime = 0;
  vtkTexture* EnvironmentTexture = nullptr;

private:
  vtkOpenGLPolyDataMapper(const vtkOpenGLPolyDataMapper&) = delete;
  void operator=(const vtkOpenGLPolyDataMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
