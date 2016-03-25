/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHardwareSelector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHardwareSelector - manager for OpenGL-based selection.
// .SECTION Description
// vtkHardwareSelector is a helper that orchestrates color buffer based
// selection. This relies on OpenGL.
// vtkHardwareSelector can be used to select visible cells or points within a
// given rectangle of the RenderWindow.
// To use it, call in order:
// \li SetRenderer() - to select the renderer in which we
// want to select the cells/points.
// \li SetArea() - to set the rectangular region in the render window to select
// in.
// \li SetFieldAssociation() -  to select the attribute to select i.e.
// cells/points etc.
// \li Finally, call Select().
// Select will cause the attached vtkRenderer to render in a special color mode,
// where each cell/point is given it own color so that later inspection of the
// Rendered Pixels can determine what cells are visible. Select() returns a new
// vtkSelection instance with the cells/points selected.
//
// Limitations:
// Antialiasing will break this class. If your graphics card settings force
// their use this class will return invalid results.
//
// Currently only cells from PolyDataMappers can be selected from. When
// vtkRenderer::Selector is non-null vtkPainterPolyDataMapper uses the
// vtkHardwareSelectionPolyDataPainter which make appropriate calls to
// BeginRenderProp(), EndRenderProp(), RenderProcessId(),
// RenderAttributeId() to render colors
// correctly. Until alternatives to vtkHardwareSelectionPolyDataPainter
// exist that can do a similar coloration of other vtkDataSet types, only
// polygonal data can be selected. If you need to select other data types,
// consider using vtkDataSetMapper and turning on it's PassThroughCellIds
// feature, or using vtkFrustumExtractor.
//
// Only Opaque geometry in Actors is selected from. Assemblies and LODMappers
// are not currently supported.
//
// During selection, visible datasets that can not be selected from are
// temporarily hidden so as not to produce invalid indices from their colors.
//
// .SECTION See Also
// vtkIdentColoredPainter

#ifndef vtkHardwareSelector_h
#define vtkHardwareSelector_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkRenderer;
class vtkRenderWindow;
class vtkSelection;
class vtkProp;
class vtkTextureObject;

class VTKRENDERINGCORE_EXPORT vtkHardwareSelector : public vtkObject
{
public:
  // Description:
  // Struct used to return information about a pixel location.
  struct PixelInformation
    {
    bool Valid;
    int ProcessID;
    int PropID;
    vtkProp* Prop;
    unsigned int CompositeID;
    vtkIdType AttributeID;
    PixelInformation():
      Valid(false),
      ProcessID(-1),
      Prop(NULL),
      CompositeID(0),
      AttributeID(-1) {}
    };

public:
  static vtkHardwareSelector* New();
  vtkTypeMacro(vtkHardwareSelector, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the renderer to perform the selection on.
  virtual void SetRenderer(vtkRenderer*);
  vtkGetObjectMacro(Renderer, vtkRenderer);

  // Description:
  // Get/Set the area to select as (xmin, ymin, xmax, ymax).
  vtkSetVector4Macro(Area, unsigned int);
  vtkGetVector4Macro(Area, unsigned int);

  // Description:
  // Set the field type to select. Valid values are
  // \li vtkDataObject::FIELD_ASSOCIATION_POINTS
  // \li vtkDataObject::FIELD_ASSOCIATION_CELLS
  // \li vtkDataObject::FIELD_ASSOCIATION_VERTICES
  // \li vtkDataObject::FIELD_ASSOCIATION_EDGES
  // \li vtkDataObject::FIELD_ASSOCIATION_ROWS
  // Currently only FIELD_ASSOCIATION_POINTS and FIELD_ASSOCIATION_CELLS are
  // supported.
  vtkSetMacro(FieldAssociation, int);
  vtkGetMacro(FieldAssociation, int);

  // Description:
  // In some parallel rendering setups, the process id for elements must be
  // obtained from the data itself, rather than the rendering process' id. In
  // that case, set this flag to ON (default OFF).
  vtkSetMacro(UseProcessIdFromData, bool);
  vtkGetMacro(UseProcessIdFromData, bool);

  // Description:
  // Perform the selection. Returns  a new instance of vtkSelection containing
  // the selection on success.
  vtkSelection* Select();

  // Description:
  // It is possible to use the vtkHardwareSelector for a custom picking. (Look
  // at vtkScenePicker). In that case instead of Select() on can use
  // CaptureBuffers() to render the selection buffers and then get information
  // about pixel locations suing GetPixelInformation(). Use ClearBuffers() to
  // clear buffers after one's done with the scene.
  // The optional final parameter maxDist will look for a cell within the specified
  // number of pixels from display_position. When using the overload with the
  // optional \c selected_position argument, selected_position is filled with
  // the position for which the PixelInformation is being returned. This is
  // useful when maxDist > 0 to determine which position's pixel information is
  // was returned.
  virtual bool CaptureBuffers();
  PixelInformation GetPixelInformation(const unsigned int display_position[2])
    { return this->GetPixelInformation(display_position, 0); }
  PixelInformation GetPixelInformation(const unsigned int display_position[2], int maxDist)
    { unsigned int temp[2]; return this->GetPixelInformation(display_position, maxDist, temp); }
  PixelInformation GetPixelInformation(const unsigned int display_position[2],
    int maxDist, unsigned int selected_position[2]);
  void ClearBuffers()
    { this->ReleasePixBuffers(); }

  // Description:
  // Called by any vtkMapper or vtkProp subclass to render a composite-index.
  // Currently indices >= 0xffffff are not supported.
  virtual void RenderCompositeIndex(unsigned int index);

  // Description:
  // Called by any vtkMapper or vtkProp subclass to render an attribute's id.
  virtual void RenderAttributeId(vtkIdType attribid);

  // Description:
  // Called by any vtkMapper or subclass to render process id. This has any
  // effect when this->UseProcessIdFromData is true.
  virtual void RenderProcessId(unsigned int processid);

  // Description:
  // Called by vtkRenderer to render the selection pass.
  // Returns the number of props rendered.
  int Render(vtkRenderer* renderer, vtkProp** propArray, int propArrayCount);

  // Description:
  // Called by the mapper (vtkHardwareSelectionPolyDataPainter) before and after
  // rendering each prop.
  virtual void BeginRenderProp();
  virtual void EndRenderProp();

  // Description:
  // Get/Set the process id. If process id < 0 (default -1), then the
  // PROCESS_PASS is not rendered.
  vtkSetMacro(ProcessID, int);
  vtkGetMacro(ProcessID, int);

  // Description:
  // Get/Set the color to be used by the prop when drawing
  vtkGetVector3Macro(PropColorValue,float);
  vtkSetVector3Macro(PropColorValue,float);

  // Description:
  // Get the current pass number.
  vtkGetMacro(CurrentPass, int);

  // Description:
  // Generates the vtkSelection from pixel buffers.
  // Requires that CaptureBuffers() has already been called.
  // Optionally you may pass a screen region (xmin, ymin, xmax, ymax)
  // to generate a selection from. The region must be a subregion
  // of the region specified by SetArea(), otherwise it will be
  // clipped to that region.
  virtual vtkSelection* GenerateSelection()
    { return GenerateSelection(this->Area); }
  virtual vtkSelection* GenerateSelection(unsigned int r[4])
    { return GenerateSelection(r[0], r[1], r[2], r[3]); }
  virtual vtkSelection* GenerateSelection(
    unsigned int x1, unsigned int y1,
    unsigned int x2, unsigned int y2);

  // Description:
  // Generates the vtkSelection from pixel buffers.
  // Same as GenerateSelection, except this one use a polygon, instead
  // of a rectangle region, and select elements inside the polygon.
  // NOTE: The CaptureBuffers() needs to be called first.
  virtual vtkSelection* GeneratePolygonSelection(
    int* polygonPoints, vtkIdType count);

  // Description:
  // returns the prop associated with a ID. This is valid only until
  // ReleasePixBuffers() gets called.
  vtkProp* GetPropFromID(int id);

//BTX
  enum PassTypes
    {
    PROCESS_PASS,
    ACTOR_PASS,
    COMPOSITE_INDEX_PASS,
    ID_LOW24,
    ID_MID24,
    ID_HIGH16,
    MAX_KNOWN_PASS = ID_HIGH16,
    MIN_KNOWN_PASS = PROCESS_PASS
    };

  static void Convert(int id, float tcoord[3])
    {
    tcoord[0] = static_cast<float>((id & 0xff)/255.0);
    tcoord[1] = static_cast<float>(((id & 0xff00) >> 8)/255.0);
    tcoord[2] = static_cast<float>(((id & 0xff0000) >> 16)/255.0);
    }

protected:
  vtkHardwareSelector();
  ~vtkHardwareSelector();

  // Called internally before and after each prop is rendered
  // for device specific configuration/preparation etc.
  virtual void BeginRenderProp(vtkRenderWindow *) = 0;
  virtual void EndRenderProp(vtkRenderWindow *) = 0;

  int Convert(unsigned long offset, unsigned char* pb)
    {
    if (!pb)
      {
      return 0;
      }
    offset = offset * 3;
    unsigned char rgb[3];
    rgb[0] = pb[offset];
    rgb[1] = pb[offset+1];
    rgb[2] = pb[offset+2];
    int val = 0;
    val |= rgb[2];
    val = val << 8;
    val |= rgb[1];
    val = val << 8;
    val |= rgb[0];
    return val;
    }

  // Description:
  // \c pos must be relative to the lower-left corner of this->Area.
  int Convert(unsigned int pos[2], unsigned char* pb)
    { return this->Convert(pos[0], pos[1], pb); }
  int Convert(int xx, int yy, unsigned char* pb)
    {
    if (!pb)
      {
      return 0;
      }
    int offset = (yy * static_cast<int>(this->Area[2]-this->Area[0]+1) + xx) * 3;
    unsigned char rgb[3];
    rgb[0] = pb[offset];
    rgb[1] = pb[offset+1];
    rgb[2] = pb[offset+2];
    int val = 0;
    val |= rgb[2];
    val = val << 8;
    val |= rgb[1];
    val = val << 8;
    val |= rgb[0];
    return val;
    }

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

  // Description:
  // Returns is the pass indicated is needed.
  virtual bool PassRequired(int pass);

  // Description:
  // After the ACTOR_PASS this return true or false depending upon whether the
  // prop was hit in the ACTOR_PASS. This makes it possible to skip props that
  // are not involved in the selection after the first pass.
  bool IsPropHit(int propid);

  // Description:
  // Return a unique ID for the prop.
  virtual int GetPropID(int idx, vtkProp* vtkNotUsed(prop))
    { return idx; }

  virtual void BeginSelection();
  virtual void EndSelection();

  virtual void SavePixelBuffer(int passNo);
  void BuildPropHitList(unsigned char* rgbData);

  // Description:
  // Clears all pixel buffers.
  void ReleasePixBuffers();
  vtkRenderer* Renderer;
  unsigned int Area[4];
  int FieldAssociation;
  bool UseProcessIdFromData;
  vtkIdType MaxAttributeId;

  // At most 10 passes.
  unsigned char* PixBuffer[10];
  int ProcessID;
  int CurrentPass;
  int InPropRender;
  int PropID;
  float PropColorValue[3];

private:
  vtkHardwareSelector(const vtkHardwareSelector&); // Not implemented.
  void operator=(const vtkHardwareSelector&); // Not implemented.

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif


