// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLLowMemoryPolyDataMapper
 * @brief   PolyDataMapper using OpenGL to render surface meshes.
 *
 * This mapper targets webassembly, mobile and other platforms where
 * memory is scarce and geometry shaders are unavailable or inefficient.
 */

#ifndef vtkOpenGLLowMemoryPolyDataMapper_h
#define vtkOpenGLLowMemoryPolyDataMapper_h

#include "vtkPolyDataMapper.h"

#include "vtkCellGraphicsPrimitiveMap.h" // For CellTypeMapperOffsets
#include "vtkDrawTexturedElements.h"     // For parent helper class
#include "vtkHardwareSelector.h"         // For ivar
#include "vtkOpenGLShaderDeclaration.h"  // For ivar
#include "vtkRenderingOpenGL2Module.h"   // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

#include <array>   // for array
#include <set>     // for set
#include <utility> // for pair
#include <vector>  // for TextureInfo

VTK_ABI_NAMESPACE_BEGIN
class vtkGenericOpenGLResourceFreeCallback;
class vtkOpenGLLowMemoryCellTypeAgent;
class vtkOpenGLLowMemoryVerticesAgent;
class vtkOpenGLLowMemoryLinesAgent;
class vtkOpenGLLowMemoryPolygonsAgent;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpenGLLowMemoryPolyDataMapper
  : public vtkPolyDataMapper
#ifndef __VTK_WRAP__
  , public vtkDrawTexturedElements
#endif
{
public:
  static vtkOpenGLLowMemoryPolyDataMapper* New();
  vtkTypeMacro(vtkOpenGLLowMemoryPolyDataMapper, vtkPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Make a shallow copy of this mapper.
   */
  void ShallowCopy(vtkAbstractMapper* m) override;

  /**
   * Unique hash based on availability of scalars, normals, tcoords, lookup tables
   * and related attributes that distinguish the rendering requirements of different
   * polydata.
   */
  MapperHashType GenerateHash(vtkPolyData* polydata) override;

  vtkPolyData* CurrentInput = nullptr;
  void RenderPiece(vtkRenderer* renderer, vtkActor* actor) override;
  virtual void RenderPieceStart(vtkRenderer* renderer, vtkActor* actor);
  virtual void RenderPieceDraw(vtkRenderer* renderer, vtkActor* actor);
  virtual void RenderPieceFinish(vtkRenderer* renderer, vtkActor* actor);

  /// Release any graphics resources associated with the \a window.
  void ReleaseGraphicsResources(vtkWindow*) override;

  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  vtkGetMacro(PopulateSelectionSettings, bool);
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  void SetPopulateSelectionSettings(bool v) { this->PopulateSelectionSettings = v; }
  void SetVBOShiftScaleMethod(int method) override;

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Used by vtkHardwareSelector to determine if the prop supports hardware
   * selection.
   */
  bool GetSupportsSelection() override { return true; }

  /// If you removed all mods, call this to go back to default setting.
  virtual void ResetModsToDefault();
  void AddMod(const std::string& className);
  void AddMods(const std::vector<std::string>& classNames);
  void RemoveMod(const std::string& className);
  void RemoveAllMods();

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
  vtkOpenGLLowMemoryPolyDataMapper();
  ~vtkOpenGLLowMemoryPolyDataMapper() override;

  enum class ShaderColorSourceAttribute
  {
    // Color is obtained by mapping point scalar array. shader will interpolate colors.
    Point,
    // Color is obtained by mapping cell scalar array. entire cell will have the same color.
    Cell,
    // Similar to `Point`, in addition, mapper already interpolated the scalars and provided a
    // ColorTextureCoordinate and a ColorTexture.
    PointTexture,
    // The color of the entire geometry is specified by the actor's vtkProperty instance.
    Uniform
  };

  enum class ShaderNormalSourceAttribute
  {
    // Uses point normals.
    Point,
    // Uses cell normals.
    Cell,
    // Shader computes a normal for the provoking vertex and passes it down to fragment shader.
    Primitive
  };

  ShaderColorSourceAttribute DetermineShaderColorSource(vtkPolyData* mesh);
  ShaderNormalSourceAttribute DetermineShaderNormalSource(vtkActor* actor, vtkPolyData* mesh);

  void MapDataArray(const char* vertexAttributeName, const char* dataArrayName,
    const char* texturename, int fieldAssociation, int componentno);
  bool IsUpToDate(vtkRenderer* renderer, vtkActor* actor);
  bool IsShaderUpToDate(vtkRenderer* renderer, vtkActor* actor);
  virtual bool IsDataObjectUpToDate();
  virtual bool IsShaderColorSourceUpToDate(vtkActor* actor);
  virtual bool IsShaderNormalSourceUpToDate(vtkActor* actor);
  void DeleteTextureBuffers();
  virtual bool BindArraysToTextureBuffers(vtkRenderer* renderer, vtkActor* actor,
    vtkCellGraphicsPrimitiveMap::CellTypeMapperOffsets& offsets);
  void InstallArrayTextureShaderDeclarations();
  virtual void UpdateShaders(vtkRenderer* renderer, vtkActor* actor);
  void ReplaceShaderValues(
    vtkRenderer* renderer, vtkActor* actor, std::string& vsSource, std::string& fsSource);
  void ReplaceShaderPosition(
    vtkRenderer* renderer, vtkActor* actor, std::string& vsSource, std::string& fsSource);
  void ReplaceShaderNormal(
    vtkRenderer* renderer, vtkActor* actor, std::string& vsSource, std::string& fsSource);
  virtual void ReplaceShaderColor(
    vtkRenderer* renderer, vtkActor* actor, std::string& vsSource, std::string& fsSource);
  void ReplaceShaderImplementationCustomUniforms(
    vtkRenderer* renderer, vtkActor* actor, std::string& vsSource, std::string& fsSource);
  void ReplaceShaderPointSize(
    vtkRenderer* renderer, vtkActor* actor, std::string& vsSource, std::string& fsSource);
  void ReplaceShaderWideLines(
    vtkRenderer* renderer, vtkActor* actor, std::string& vsSource, std::string& fsSource);
  void ReplaceShaderEdges(
    vtkRenderer* renderer, vtkActor* actor, std::string& vsSource, std::string& fsSource);
  void ReplaceShaderSelection(
    vtkRenderer* renderer, vtkActor* actor, std::string& vsSource, std::string& fsSource);
  void ReplaceShaderTCoord(
    vtkRenderer* renderer, vtkActor* actor, std::string& vsSource, std::string& fsSource);
  void ReplaceShaderClip(
    vtkRenderer* renderer, vtkActor* actor, std::string& vsSource, std::string& fsSource);
  void SetShaderParameters(vtkRenderer* renderer, vtkActor* actor);
  // compute and set the maximum point and cell ID used in selection
  void UpdateMaximumPointCellIds(vtkRenderer* ren, vtkActor* actor);

  bool GetCoordShiftAndScaleEnabled();
  int GetCoordShiftAndScaleMethod();
  void SetShiftValues(double x, double y, double z);
  void SetScaleValues(double x, double y, double z);
  void ComputeShiftScale(vtkRenderer* renderer, vtkActor* actor, vtkDataArray* arrays);
  void ComputeCameraBasedShiftScale(vtkRenderer* renderer, vtkActor* actor, vtkPolyData* mesh);
  void ComputeShiftScaleTransform(vtkRenderer* renderer, vtkActor* actor);
  virtual void UpdateShiftScale(vtkRenderer* renderer, vtkActor* actor);

  virtual vtkDataArray* GetColors(vtkPolyData* mesh);
  vtkDataArray* GetPointNormals(vtkPolyData* mesh);
  vtkDataArray* GetPointTangents(vtkPolyData* mesh);
  vtkDataArray* GetTextureCoordinates(vtkPolyData* mesh);
  vtkDataArray* GetColorTextureCoordinates(vtkPolyData* mesh);
  vtkDataArray* GetCellNormals(vtkPolyData* mesh);

  using TextureInfo = std::pair<vtkTexture*, std::string>;
  bool HaveTextures(vtkActor* actor);
  unsigned int GetNumberOfTextures(vtkActor* actor);
  std::vector<TextureInfo> GetTextures(vtkActor* actor);
  std::pair<std::string, std::string> GetTextureCoordinateAndSamplerBufferNames(const char* tname);

  void UpdatePBRStateCache(vtkRenderer* renderer, vtkActor* actor);
  void UpdateGLSLMods(vtkRenderer* renderer, vtkActor* actor);

  class ExtraAttributeValue
  {
  public:
    std::string DataArrayName;
    int FieldAssociation;
    int ComponentNumber;
    std::string TextureName;
  };
  std::map<std::string, ExtraAttributeValue> ExtraAttributes;

  /// @name Color and Normal sources
  ShaderColorSourceAttribute ShaderColorSource = ShaderColorSourceAttribute::Uniform;
  ShaderNormalSourceAttribute ShaderNormalSource = ShaderNormalSourceAttribute::Primitive;

  /// @name Mods
  /// These are the names of classes which are subclasess of vtkGLSLRuntimeModBase.
  /// The mods will be loaded one by one and applied in the order they were added.
  std::vector<std::string> ModNames;
  std::set<std::string> ModNamesUnique;
  static std::vector<std::string> DefaultModNames;

  /// @name Shader declarations in an organized form
  using GLSLAttributeType = vtkOpenGLShaderDeclaration::GLSLAttributeType;
  using GLSLDataType = vtkOpenGLShaderDeclaration::GLSLDataType;
  using GLSLPrecisionType = vtkOpenGLShaderDeclaration::GLSLPrecisionType;
  using GLSLQualifierType = vtkOpenGLShaderDeclaration::GLSLQualifierType;
  std::vector<vtkOpenGLShaderDeclaration> ShaderDecls;

  /// @name Last render information used to check whether shader program needs an update.
  vtkNew<vtkInformation> LastRenderPassInfo;
  int LastSelectionState = vtkHardwareSelector::MIN_KNOWN_PASS - 1;
  vtkTimeStamp RenderTimeStamp;
  vtkTimeStamp SelectionStateTimeStamp;
  vtkTimeStamp ShaderBuildTimeStamp;
  vtkTimeStamp ShiftScaleTimeStamp;

  /// @name Coordinate shift scale for datasets whose bounding box is far from origin.
  std::array<double, 3> ShiftValues;
  std::array<double, 3> ScaleValues;
  bool CoordinateShiftAndScaleInUse = false;
  vtkNew<vtkTransform> SSInverseTransform; // Inverse transform which can undo shift + scale.
  vtkNew<vtkMatrix4x4> SSMatrix;           // Transpose of the above inverse transform.

  struct CellGroupInformation
  {
    vtkCellGraphicsPrimitiveMap::CellTypeMapperOffsets Offsets;
    vtkIdType NumberOfElements = 0;
    bool CanRender = false;
    bool UsesEdgeValueBuffer = false;
    bool UsesCellMapBuffer = false;
    friend std::ostream& operator<<(std::ostream& os, const CellGroupInformation& cg)
    {
      os << cg.Offsets << '\n'
         << "NumberOfElements: " << cg.NumberOfElements << '\n'
         << "CanRender: " << cg.CanRender << '\n'
         << "UsesEdgeValueBuffer: " << cg.UsesEdgeValueBuffer << '\n'
         << "UsesCellMapBuffer: " << cg.UsesCellMapBuffer << '\n';
      return os;
    }
  };
  /// @name Primitives
  /// @brief vtkPolyData has four celltypes. Each gets it's own cell type mapper with it's own cell
  /// groups.
  struct PrimitiveInformation
  {
    std::unique_ptr<vtkOpenGLLowMemoryCellTypeAgent> Agent;
    std::function<vtkCellGraphicsPrimitiveMap::PrimitiveDescriptor(vtkPolyData*)> GeneratorFunction;
    std::vector<CellGroupInformation> CellGroups;
  };
  std::array<PrimitiveInformation, 4> Primitives;
  bool DrawingVertices = false;
  bool HasColors = false;
  bool HasTangents = false;
  bool HasPointNormals = false;
  bool HasCellNormals = false;
  bool HasPointTextureCoordinates = false;
  // if set to true, tcoords will be passed to the
  // VBO even if the mapper knows of no texture maps
  // normally tcoords are only added to the VBO if the
  // mapper has identified a texture map as well.
  bool ForceTextureCoordinates = false;

  /// @name ColorTextureMap
  vtkOpenGLTexture* InternalColorTexture = nullptr;

  /// @name Selection data
  bool PopulateSelectionSettings = true;
  bool PointPicking = false;

  /// @name Cached PBR information
  bool HasAnisotropy = false;
  bool HasClearCoat = false;
  bool UsesNormalMap = false;
  bool UsesCoatNormalMap = false;
  bool UsesRotationMap = false;
  vtkTimeStamp PBRStateTimeStamp;

private:
  vtkOpenGLLowMemoryPolyDataMapper(const vtkOpenGLLowMemoryPolyDataMapper&) = delete;
  void operator=(const vtkOpenGLLowMemoryPolyDataMapper&) = delete;
  friend class vtkOpenGLLowMemoryCellTypeAgent;
  friend class vtkOpenGLLowMemoryVerticesAgent;
  friend class vtkOpenGLLowMemoryLinesAgent;
  friend class vtkOpenGLLowMemoryPolygonsAgent;

  vtkNew<vtkMatrix4x4> TempMatrix4;
};

VTK_ABI_NAMESPACE_END
#endif // vtkOpenGLLowMemoryPolyDataMapper_h
