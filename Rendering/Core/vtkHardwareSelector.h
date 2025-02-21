// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/*
 * @class   vtkHardwareSelector
 * @brief   manager for OpenGL-based selection.
 *
 * vtkHardwareSelector is a helper that orchestrates color buffer based
 * selection.
 * vtkHardwareSelector can be used to select visible cells or points within a
 * given rectangle of the RenderWindow.
 * To use it, call in order:
 * \li SetRenderer() - to select the renderer in which we
 * want to select the cells/points.
 * \li SetArea() - to set the rectangular region in the render window to select
 * in.
 * \li SetFieldAssociation() -  to select the attribute to select i.e.
 * cells/points etc.
 * \li Finally, call Select().
 * Select will cause the attached vtkRenderer to render in a special color mode,
 * where each cell/point is given it own color so that later inspection of the
 * Rendered Pixels can determine what cells are visible. Select() returns a new
 * vtkSelection instance with the cells/points selected.
 *
 * Limitations:
 * Antialiasing will break this class. If your graphics card settings force
 * their use this class will return invalid results.
 *
 * Only Opaque geometry in Actors is selected from. Assemblies and LODMappers
 * are not currently supported.
 *
 * During selection, visible datasets that can not be selected from are
 * temporarily hidden so as not to produce invalid indices from their colors.
 *
 *
 * The basic approach this class uses is to invoke render multiple times
 * (passes) and have the mappers render pass specific information into
 * the color buffer. For example during the ACTOR_PASS a mapper is
 * supposed to render it's actor's id into the color buffer as a RGB
 * value where R is the lower 8 bits, G is the next 8, etc. Giving us 24
 * bits of unsigned int range.
 *
 * The same concept applies to the COMPOSITE_INDEX_PASS and the point and
 * cell ID passes. As points and cells can easily exceed the 24 bit range
 * of the color buffer we break them into two 24 bit passes for a total
 * of 48 bits of range.
 *
 * During each pass the mappers render their data into the color buffer,
 * the hardware selector grabs that buffer and then invokes
 * ProcessSelectorPixelBuffer on all of the hit props. Giving them, and
 * their mappers, a chance to modify the pixel buffer.
 *
 * Most mappers use this ProcessSelectorPixelBuffers pass to take when
 * they rendered into the color buffer and convert it into what the
 * hardware selector is expecting. This is because in some cases it is
 * far easier and faster to  render something else, such as
 * gl_PrimitiveID or gl_VertexID and then in the processing convert those
 * values to the appropriate VTK values.
 *
 * NOTE:  The goal is for mappers to support hardware selection without
 * having to rebuild any of their VBO/IBOs to maintain fast picking
 * performance.
 *
 * NOTE: This class has a complex interaction with parallel compositing
 * techniques such as IceT that are used on supercomputers. In those
 * cases the local nodes render each pass, process it, send it to icet
 * which composites it, and then must copy the result back to the hardware
 * selector. Be aware of these interactions  if you work on this class.
 *
 * NOTE: many mappers support remapping arrays from their local value to
 * some other provided value. For example ParaView when creating a
 * polydata from an unstructured grid will create point and cell data
 * arrays on the polydata that may the polydata point and cell IDs back
 * to the original unstructured grid's point and cell IDs. The hardware
 * selection process honors those arrays and will provide the original
 * unstructured grid point and cell ID when a selection is made.
 * Likewise there are process and composite arrays that most mappers
 * support that allow for parallel data generation, delivery, and local
 * rendering while preserving the original process and composite values
 * from when the data was distributed. Be aware the process array is a
 * point data while the composite array is a cell data.
 *
 * TODO: This whole selection process could be nicely encapsulated as a
 * RenderPass that internally renders multiple times with different
 * settings. That would be my suggestion for the future.
 *
 * TODO: The pick method build into renderer could use the ACTOR pass of
 * this class to do it's work eliminating some confusion and duplicate
 * code paths.
 *
 * TODO: I am not sure where the composite array indirection is used.
 *
 *
 * @sa
 * vtkOpenGLHardwareSelector
 */

#ifndef vtkHardwareSelector_h
#define vtkHardwareSelector_h

#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro

#include <string> // for std::string

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderer;
class vtkRenderWindow;
class vtkSelection;
class vtkProp;
class vtkTextureObject;

class VTKRENDERINGCORE_EXPORT vtkHardwareSelector : public vtkObject
{
public:
  ///@{
  /**
   * Struct used to return information about a pixel location.
   */
  struct PixelInformation
  {
    bool Valid;
    int ProcessID;
    int PropID;
    vtkProp* Prop;
    unsigned int CompositeID;
    vtkIdType AttributeID;
    vtkIdType CellGridCellTypeID;
    vtkIdType CellGridSourceSpecID;
    vtkIdType CellGridTupleID;
    PixelInformation()
      : Valid(false)
      , ProcessID(-1)
      , PropID(-1)
      , Prop(nullptr)
      , CompositeID(0)
      , AttributeID(-1)
      , CellGridCellTypeID(-1)
      , CellGridSourceSpecID(-1)
      , CellGridTupleID(-1)
    {
    }
  };
  ///@}

  static vtkHardwareSelector* New();
  vtkTypeMacro(vtkHardwareSelector, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the renderer to perform the selection on.
   */
  virtual void SetRenderer(vtkRenderer*);
  vtkGetObjectMacro(Renderer, vtkRenderer);
  ///@}

  ///@{
  /**
   * Get/Set the area to select as (xmin, ymin, xmax, ymax).
   */
  vtkSetVector4Macro(Area, unsigned int);
  vtkGetVector4Macro(Area, unsigned int);
  ///@}

  ///@{
  /**
   * Set the field type to select. Valid values are
   * \li vtkDataObject::FIELD_ASSOCIATION_POINTS
   * \li vtkDataObject::FIELD_ASSOCIATION_CELLS
   * \li vtkDataObject::FIELD_ASSOCIATION_VERTICES
   * \li vtkDataObject::FIELD_ASSOCIATION_EDGES
   * \li vtkDataObject::FIELD_ASSOCIATION_ROWS
   * Currently only FIELD_ASSOCIATION_POINTS and FIELD_ASSOCIATION_CELLS are
   * supported.
   */
  vtkSetMacro(FieldAssociation, int);
  vtkGetMacro(FieldAssociation, int);
  ///@}

  ///@{
  /**
   * In some parallel rendering setups, the process id for elements must be
   * obtained from the data itself, rather than the rendering process' id. In
   * that case, set this flag to ON (default OFF).
   */
  vtkSetMacro(UseProcessIdFromData, bool);
  vtkGetMacro(UseProcessIdFromData, bool);
  ///@}

  /**
   * Perform the selection. Returns a new instance of vtkSelection containing
   * the selection on success.
   */
  VTK_NEWINSTANCE vtkSelection* Select();

  ///@{
  /**
   * It is possible to use the vtkHardwareSelector for a custom picking. (Look
   * at vtkScenePicker). In that case instead of Select() on can use
   * CaptureBuffers() to render the selection buffers and then get information
   * about pixel locations using GetPixelInformation(). Use ClearBuffers() to
   * clear buffers after one's done with the scene.
   * The optional final parameter maxDist will look for a cell within the specified
   * number of pixels from display_position. When using the overload with the
   * optional \c selected_position argument, selected_position is filled with
   * the position for which the PixelInformation is being returned. This is
   * useful when maxDist > 0 to determine which position's pixel information is
   * was returned.
   */
  virtual bool CaptureBuffers();
  PixelInformation GetPixelInformation(const unsigned int display_position[2])
  {
    return this->GetPixelInformation(display_position, 0);
  }
  PixelInformation GetPixelInformation(const unsigned int display_position[2], int maxDist)
  {
    unsigned int temp[2];
    return this->GetPixelInformation(display_position, maxDist, temp);
  }
  virtual PixelInformation GetPixelInformation(
    const unsigned int display_position[2], int maxDist, unsigned int selected_position[2]);
  void ClearBuffers() { this->ReleasePixBuffers(); }
  // raw is before processing
  unsigned char* GetRawPixelBuffer(int passNo) { return this->RawPixBuffer[passNo]; }
  unsigned char* GetPixelBuffer(int passNo) { return this->PixBuffer[passNo]; }
  ///@}

  /**
   * Called by any vtkMapper or vtkProp subclass to render a composite-index.
   * Currently indices >= 0xffffff are not supported.
   */
  virtual void RenderCompositeIndex(unsigned int index);

  ///@{
  /**
   * Called by any vtkMapper or vtkProp subclass to indicate the
   * maximum cell or point attribute ID it uses. These values are
   * used for determining if the POINT_ID_HIGH or CELL_ID_HIGH
   * passes are required.
   */
  virtual void UpdateMaximumCellId(vtkIdType attribid);
  virtual void UpdateMaximumPointId(vtkIdType attribid);
  virtual void UpdateMaximumCellGridTupleId(vtkIdType attribid);
  ///@}

  /**
   * Called by any vtkMapper or subclass to render process id. This has any
   * effect when this->UseProcessIdFromData is true.
   */
  virtual void RenderProcessId(unsigned int processid);

  /**
   * Called by vtkRenderer to render the selection pass.
   * Returns the number of props rendered.
   */
  int Render(vtkRenderer* renderer, vtkProp** propArray, int propArrayCount);

  ///@{
  /**
   * Get/Set to only do the actor pass. If true all other passes will be
   * skipped resulting in a faster pick.
   */
  vtkGetMacro(ActorPassOnly, bool);
  vtkSetMacro(ActorPassOnly, bool);
  ///@}

  ///@{
  /**
   * Get/Set to capture the zvalue. If true the closest zvalue is
   * stored for each prop that is in the selection. ZValue in this
   * case is the value from the zbuffer which can be used in
   * coordinate conversions
   */
  vtkGetMacro(CaptureZValues, bool);
  vtkSetMacro(CaptureZValues, bool);
  ///@}

  ///@{
  /**
   * Called by the mapper before and after rendering each prop.
   */
  virtual void BeginRenderProp();
  virtual void EndRenderProp();
  ///@}

  ///@{
  /**
   * Get/Set the process id. If process id < 0 (default -1), then the
   * PROCESS_PASS is not rendered.
   */
  vtkSetMacro(ProcessID, int);
  vtkGetMacro(ProcessID, int);
  ///@}

  ///@{
  /**
   * Get/Set the color to be used by the prop when drawing
   */
  vtkGetVector3Macro(PropColorValue, float);
  vtkSetVector3Macro(PropColorValue, float);
  void SetPropColorValue(vtkIdType val);
  ///@}

  ///@{
  /**
   * Get the current pass number.
   */
  vtkGetMacro(CurrentPass, int);
  ///@}

  /**
   * Generates the vtkSelection from pixel buffers.
   * Requires that CaptureBuffers() has already been called.
   * Optionally you may pass a screen region (xmin, ymin, xmax, ymax)
   * to generate a selection from. The region must be a subregion
   * of the region specified by SetArea(), otherwise it will be
   * clipped to that region.
   */
  virtual vtkSelection* GenerateSelection() { return GenerateSelection(this->Area); }
  virtual vtkSelection* GenerateSelection(unsigned int r[4])
  {
    return GenerateSelection(r[0], r[1], r[2], r[3]);
  }
  virtual vtkSelection* GenerateSelection(
    unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2);

  /**
   * Generates the vtkSelection from pixel buffers.
   * Same as GenerateSelection, except this one use a polygon, instead
   * of a rectangle region, and select elements inside the polygon.
   * NOTE: The CaptureBuffers() needs to be called first.
   */
  virtual vtkSelection* GeneratePolygonSelection(int* polygonPoints, vtkIdType count);

  /**
   * returns the prop associated with a ID. This is valid only until
   * ReleasePixBuffers() gets called.
   */
  virtual vtkProp* GetPropFromID(int id);

  // it is very critical that these passes happen in the right order
  // this is because of two complexities
  //
  // Compositing engines such as iceT send each pass as it
  // renders. This means
  //
  // Mappers use point Ids or cell Id to update the process
  // and composite ids. So the point and cell id passes
  // have to happen before the last process and compoite
  // passes respectively
  //
  //
  enum PassTypes
  {
    // always must be first so that the prop IDs are set
    ACTOR_PASS,
    // must always be second for composite mapper
    COMPOSITE_INDEX_PASS,

    POINT_ID_LOW24,
    POINT_ID_HIGH24, // if needed
    PROCESS_PASS,    // must be after point id pass

    CELL_ID_LOW24,
    CELL_ID_HIGH24, // if needed

    CELLGRID_CELL_TYPE_INDEX_PASS,
    CELLGRID_SOURCE_INDEX_PASS,
    CELLGRID_TUPLE_ID_LOW24,
    CELLGRID_TUPLE_ID_HIGH24,

    MAX_KNOWN_PASS = CELLGRID_TUPLE_ID_HIGH24,
    MIN_KNOWN_PASS = ACTOR_PASS
  };

  /**
   * Convert a PassTypes enum value to a human readable string.
   */
  std::string PassTypeToString(PassTypes type);

  static void Convert(vtkIdType id, float tcoord[3])
  {
    tcoord[0] = static_cast<float>((id & 0xff) / 255.0);
    tcoord[1] = static_cast<float>(((id & 0xff00) >> 8) / 255.0);
    tcoord[2] = static_cast<float>(((id & 0xff0000) >> 16) / 255.0);
  }

  // grab the pixel buffer and save it
  // typically called internally
  virtual void SavePixelBuffer(int passNo);

  // does the selection process have high cell data
  // requiring a high24 pass
  bool HasHighCellIds();

  // does the selection process have high point data
  // requiring a high24 pass
  bool HasHighPointIds();

  // deos the selection process have high cell grid tuple ids
  // requiring a high24 pass
  bool HasHighCellGridTupleIds();

protected:
  vtkHardwareSelector();
  ~vtkHardwareSelector() override;

  // Used to notify subclasses when a capture pass is occurring.
  virtual void PreCapturePass(int pass) { (void)pass; }
  virtual void PostCapturePass(int pass) { (void)pass; }

  // Called internally before and after each prop is rendered
  // for device specific configuration/preparation etc.
  virtual void BeginRenderProp(vtkRenderWindow*) = 0;
  virtual void EndRenderProp(vtkRenderWindow*) = 0;

  double GetZValue(int propid);

  int Convert(unsigned long offset, unsigned char* pb)
  {
    if (!pb)
    {
      return 0;
    }
    offset = offset * 3;
    unsigned char rgb[3];
    rgb[0] = pb[offset];
    rgb[1] = pb[offset + 1];
    rgb[2] = pb[offset + 2];
    int val = 0;
    val |= rgb[2];
    val = val << 8;
    val |= rgb[1];
    val = val << 8;
    val |= rgb[0];
    return val;
  }

  ///@{
  /**
   * \c pos must be relative to the lower-left corner of this->Area.
   */
  int Convert(unsigned int pos[2], unsigned char* pb) { return this->Convert(pos[0], pos[1], pb); }
  virtual int Convert(int xx, int yy, unsigned char* pb)
  {
    if (!pb)
    {
      return 0;
    }
    int offset = (yy * static_cast<int>(this->Area[2] - this->Area[0] + 1) + xx) * 3;
    unsigned char rgb[3];
    rgb[0] = pb[offset];
    rgb[1] = pb[offset + 1];
    rgb[2] = pb[offset + 2];
    int val = 0;
    val |= rgb[2];
    val = val << 8;
    val |= rgb[1];
    val = val << 8;
    val |= rgb[0];
    return val;
  }
  ///@}

  vtkIdType GetID(int low24, int mid24, int high16)
  {
    vtkIdType val = 0;
    val |= high16;
    val = val << 24;
    val |= mid24;
    val = val << 24;
    val |= low24;
    return val;
  }

  /**
   * Returns is the pass indicated is needed.
   */
  virtual bool PassRequired(int pass);

  /**
   * After the ACTOR_PASS this return true or false depending upon whether the
   * prop was hit in the ACTOR_PASS. This makes it possible to skip props that
   * are not involved in the selection after the first pass.
   */
  bool IsPropHit(int propid);

  /**
   * Return a unique ID for the prop.
   */
  virtual int GetPropID(int idx, vtkProp* vtkNotUsed(prop)) { return idx; }

  virtual void BeginSelection();
  virtual void EndSelection();

  virtual void ProcessPixelBuffers();
  void BuildPropHitList(unsigned char* rgbData);

  ///@{
  /**
   * Clears all pixel buffers.
   */
  virtual void ReleasePixBuffers();
  vtkRenderer* Renderer;
  unsigned int Area[4];
  int FieldAssociation;
  bool UseProcessIdFromData;
  vtkIdType MaximumPointId;
  vtkIdType MaximumCellId;
  vtkIdType MaximumCellGridTupleId;
  ///@}

  // At most 11 passes.
  unsigned char* PixBuffer[11];
  unsigned char* RawPixBuffer[11];
  int ProcessID;
  int CurrentPass;
  int Iteration;
  int InPropRender;
  int PropID;
  float PropColorValue[3];

  bool ActorPassOnly;

  bool CaptureZValues;

private:
  vtkHardwareSelector(const vtkHardwareSelector&) = delete;
  void operator=(const vtkHardwareSelector&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

VTK_ABI_NAMESPACE_END
#endif
