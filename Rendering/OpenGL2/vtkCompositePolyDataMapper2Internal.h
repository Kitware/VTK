// this class encapsulates values tied to a
// polydata
class vtkCompositeMapperHelperData
{
public:
  vtkPolyData* Data;
  unsigned int FlatIndex;
  double Opacity;
  bool IsOpaque;
  bool Visibility;
  bool Pickability;
  bool OverridesColor;
  vtkColor3d AmbientColor;
  vtkColor3d DiffuseColor;

  bool Marked;

  unsigned int StartVertex;
  unsigned int NextVertex;

  // point line poly strip edge stripedge
  unsigned int StartIndex[vtkOpenGLPolyDataMapper::PrimitiveEnd];
  unsigned int NextIndex[vtkOpenGLPolyDataMapper::PrimitiveEnd];

  // stores the mapping from vtk cells to gl_PrimitiveId
  vtkNew<vtkOpenGLCellToVTKCellMap> CellCellMap;
};

//===================================================================
// We define a helper class that is a subclass of vtkOpenGLPolyDataMapper
class VTKRENDERINGOPENGL2_EXPORT vtkCompositeMapperHelper2 : public vtkOpenGLPolyDataMapper
{
public:
  static vtkCompositeMapperHelper2* New();
  vtkTypeMacro(vtkCompositeMapperHelper2, vtkOpenGLPolyDataMapper);

  void SetParent(vtkCompositePolyDataMapper2* p) { this->Parent = p; }

  vtkCompositeMapperHelperData* AddData(vtkPolyData* pd, unsigned int flatIndex);

  // Description:
  // Implemented by sub classes. Actual rendering is done here.
  void RenderPiece(vtkRenderer* ren, vtkActor* act) override;

  // keep track of what data is being used as the multiblock
  // can change
  void ClearMark();
  void RemoveUnused();
  bool GetMarked() { return this->Marked; }
  void SetMarked(bool v) { this->Marked = v; }

  /**
   * Accessor to the ordered list of PolyData that we last drew.
   */
  std::vector<vtkPolyData*> GetRenderedList() { return this->RenderedList; }

  /**
   * allows a mapper to update a selections color buffers
   * Called from a prop which in turn is called from the selector
   */
  void ProcessSelectorPixelBuffers(
    vtkHardwareSelector* sel, std::vector<unsigned int>& pixeloffsets, vtkProp* prop) override;

  virtual void ProcessCompositePixelBuffers(vtkHardwareSelector* sel, vtkProp* prop,
    vtkCompositeMapperHelperData* hdata, std::vector<unsigned int>& mypixels);

protected:
  vtkCompositePolyDataMapper2* Parent;
  std::map<vtkPolyData*, vtkCompositeMapperHelperData*> Data;

  bool Marked;

  vtkCompositeMapperHelper2() { this->Parent = nullptr; };
  ~vtkCompositeMapperHelper2() override;

  void DrawIBO(vtkRenderer* ren, vtkActor* actor, int primType, vtkOpenGLHelper& CellBO,
    GLenum mode, int pointSize);

  virtual void SetShaderValues(
    vtkShaderProgram* prog, vtkCompositeMapperHelperData* hdata, size_t primOffset);

  /**
   * Make sure appropriate shaders are defined, compiled and bound.  This method
   * orchistrates the process, much of the work is done in other methods
   */
  virtual void UpdateShaders(vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* act) override;

  // Description:
  // Perform string replacements on the shader templates, called from
  // ReplaceShaderValues
  void ReplaceShaderColor(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act) override;

  // Description:
  // Build the VBO/IBO, called by UpdateBufferObjects
  void BuildBufferObjects(vtkRenderer* ren, vtkActor* act) override;
  virtual void AppendOneBufferObject(vtkRenderer* ren, vtkActor* act,
    vtkCompositeMapperHelperData* hdata, vtkIdType& flat_index, std::vector<unsigned char>& colors,
    std::vector<float>& norms);

  // Description:
  // Returns if we can use texture maps for scalar coloring. Note this doesn't
  // say we "will" use scalar coloring. It says, if we do use scalar coloring,
  // we will use a texture. Always off for this mapper.
  int CanUseTextureMapForColoring(vtkDataObject*) override;

  std::vector<unsigned int> VertexOffsets;

  // vert line poly strip edge stripedge
  std::vector<unsigned int> IndexArray[PrimitiveEnd];

  void RenderPieceDraw(vtkRenderer* ren, vtkActor* act) override;

  bool PrimIDUsed;
  bool OverideColorUsed;

  vtkHardwareSelector* CurrentSelector;

  // bookkeeping required by vtkValuePass
  std::vector<vtkPolyData*> RenderedList;

  // used by the hardware selector
  std::vector<std::vector<unsigned int> > PickPixels;

  std::map<vtkAbstractArray*, vtkDataArray*> ColorArrayMap;

private:
  vtkCompositeMapperHelper2(const vtkCompositeMapperHelper2&) = delete;
  void operator=(const vtkCompositeMapperHelper2&) = delete;
};
// VTK-HeaderTest-Exclude: vtkCompositePolyDataMapper2Internal.h
