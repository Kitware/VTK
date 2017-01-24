// this class encapsulates values tied to a
// polydata
class vtkCompositeMapperHelperData
{
public:
  vtkPolyData *Data;
  unsigned int FlatIndex;
  double Opacity;
  bool Visibility;
  bool OverridesColor;
  vtkColor3d AmbientColor;
  vtkColor3d DiffuseColor;

  bool Marked;

  unsigned int StartVertex;
  unsigned int NextVertex;

  // point line poly strip edge stripedge
  unsigned int StartIndex[vtkOpenGLPolyDataMapper::PrimitiveEnd];
  unsigned int NextIndex[vtkOpenGLPolyDataMapper::PrimitiveEnd];

  // Point Line Poly Strip end
  size_t PrimOffsets[5];

  bool Different(
    vtkCompositeMapperHelperData *next,
    vtkHardwareSelector *selector,
    int primType)
  {
    return
      (selector &&
        selector->GetCurrentPass() ==
            vtkHardwareSelector::COMPOSITE_INDEX_PASS) ||
      this->Opacity != next->Opacity ||
      this->Visibility != next->Visibility ||
      this->OverridesColor != next->OverridesColor ||
      this->AmbientColor != next->AmbientColor ||
      this->DiffuseColor != next->DiffuseColor ||
      (primType >= 0 && primType <= 3 &&
        this->PrimOffsets[primType+1] != next->PrimOffsets[primType]);
  }

};

//===================================================================
// We define a helper class that is a subclass of vtkOpenGLPolyDataMapper
class VTKRENDERINGOPENGL2_EXPORT vtkCompositeMapperHelper2 : public vtkOpenGLPolyDataMapper
{
public:
  static vtkCompositeMapperHelper2* New();
  vtkTypeMacro(vtkCompositeMapperHelper2, vtkOpenGLPolyDataMapper);

  void SetParent(vtkCompositePolyDataMapper2 *p) {
    this->Parent = p; }

  vtkCompositeMapperHelperData *AddData(vtkPolyData *pd, unsigned int flatIndex);

  // Description:
  // Implemented by sub classes. Actual rendering is done here.
  void RenderPiece(vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;

  // keep track of what data is being used as the multiblock
  // can change
  void ClearMark();
  void RemoveUnused();
  bool GetMarked() { return this->Marked; }
  void SetMarked(bool v) { this->Marked = v; }

  /**
   * Accessor to the ordered list of PolyData that we last drew.
   */
  std::vector<vtkPolyData*> GetRenderedList(){ return this->RenderedList; }

protected:
  vtkCompositePolyDataMapper2 *Parent;
  std::map<vtkPolyData *, vtkCompositeMapperHelperData *> Data;

  bool Marked;

  vtkCompositeMapperHelper2()
  {
    this->Parent = 0;
  };
  ~vtkCompositeMapperHelper2() VTK_OVERRIDE;

  void DrawIBO(
    vtkRenderer* ren, vtkActor *actor,
    int primType,
    vtkOpenGLHelper &CellBO,
    GLenum mode,
    int pointSize);

  void SetShaderValues(
    vtkShaderProgram *prog,
    vtkCompositeMapperHelperData *hdata,
    size_t primOffset);

  // Description:
  // Perform string replacments on the shader templates, called from
  // ReplaceShaderValues
  void ReplaceShaderColor(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;

  // Description:
  // Determine if the buffer objects need to be rebuilt
  bool GetNeedToRebuildBufferObjects(vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;

  // Description:
  // Build the VBO/IBO, called by UpdateBufferObjects
  void BuildBufferObjects(vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;
  virtual void AppendOneBufferObject(vtkRenderer *ren,
    vtkActor *act, vtkCompositeMapperHelperData *hdata,
    unsigned int &flat_index,
    std::vector<unsigned char> &colors,
    std::vector<float> &norms);

  // Description:
  // Returns if we can use texture maps for scalar coloring. Note this doesn't
  // say we "will" use scalar coloring. It says, if we do use scalar coloring,
  // we will use a texture. Always off for this mapper.
  int CanUseTextureMapForColoring(vtkDataObject*) VTK_OVERRIDE;

  std::vector<unsigned int> VertexOffsets;

  // vert line poly strip edge stripedge
  std::vector<unsigned int> IndexArray[PrimitiveEnd];

  void RenderPieceDraw(vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;

  bool PrimIDUsed;
  bool OverideColorUsed;

  vtkHardwareSelector *CurrentSelector;
  double CurrentAmbientIntensity;
  double CurrentDiffuseIntensity;

  std::vector<vtkPolyData*> RenderedList;

private:
  vtkCompositeMapperHelper2(const vtkCompositeMapperHelper2&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCompositeMapperHelper2&) VTK_DELETE_FUNCTION;
};
// VTK-HeaderTest-Exclude: vtkCompositePolyDataMapper2Internal.h
