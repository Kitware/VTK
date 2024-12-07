// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolyDataMapper
 * @brief   map vtkPolyData to graphics primitives
 *
 * vtkPolyDataMapper is a class that maps polygonal data (i.e., vtkPolyData)
 * to graphics primitives. vtkPolyDataMapper serves as a superclass for
 * device-specific poly data mappers, that actually do the mapping to the
 * rendering/graphics hardware/software.
 */

#ifndef vtkPolyDataMapper_h
#define vtkPolyDataMapper_h

#include "vtkMapper.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

#include <cstdint> // For uintptr_t

VTK_ABI_NAMESPACE_BEGIN
class vtkPolyData;
class vtkRenderer;
class vtkRenderWindow;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkPolyDataMapper : public vtkMapper
{
public:
  static vtkPolyDataMapper* New();
  vtkTypeMacro(vtkPolyDataMapper, vtkMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Implemented by sub classes. Actual rendering is done here.
   */
  virtual void RenderPiece(vtkRenderer*, vtkActor*) {}

  /**
   * This calls RenderPiece (in a for loop if streaming is necessary).
   */
  void Render(vtkRenderer* ren, vtkActor* act) override;

  using MapperHashType = std::uintptr_t;
  /**
   * This hash integer is computed by concrete graphics implementation of this class.
   * For two different polydata instances, concrete implementations MUST return identical value,
   * if both the polydata can be batched together for device uploads.
   *
   * @note: For example, the OpenGL impl is capable of grouping polydata
   * that are similar in terms of the availability of scalars, normals and tcoords.
   */
  virtual MapperHashType GenerateHash(vtkPolyData*) { return 0; }

  ///@{
  /**
   * Specify the input data to map.
   */
  void SetInputData(vtkPolyData* in);
  vtkPolyData* GetInput();
  ///@}

  ///@{
  /**
   * Bring this algorithm's outputs up-to-date.
   */
  void Update(int port) override;
  void Update() override;
  vtkTypeBool Update(int port, vtkInformationVector* requests) override;
  vtkTypeBool Update(vtkInformation* requests) override;
  ///@}

  ///@{
  /**
   * If you want only a part of the data, specify by setting the piece.
   */
  vtkSetMacro(Piece, int);
  vtkGetMacro(Piece, int);
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);
  vtkSetMacro(NumberOfSubPieces, int);
  vtkGetMacro(NumberOfSubPieces, int);
  ///@}

  ///@{
  /**
   * Set the number of ghost cells to return.
   */
  vtkSetMacro(GhostLevel, int);
  vtkGetMacro(GhostLevel, int);
  ///@}

  ///@{
  /**
   * Accessors / Mutators for handling seams on wrapping surfaces. Letters U and V stand for
   * texture coordinates (u,v).
   *
   * @note Implementation taken from the work of Marco Tarini:
   * Cylindrical and Toroidal Parameterizations Without Vertex Seams
   * Journal of Graphics Tools, 2012, number 3, volume 16, pages 144-150.
   */
  vtkSetMacro(SeamlessU, bool);
  vtkGetMacro(SeamlessU, bool);
  vtkBooleanMacro(SeamlessU, bool);
  vtkSetMacro(SeamlessV, bool);
  vtkGetMacro(SeamlessV, bool);
  vtkBooleanMacro(SeamlessV, bool);
  ///@}

  ///@{
  /**
   * By default, this class uses the dataset's point and cell ids during
   * rendering. However, one can override those by specifying cell and point
   * data arrays to use instead. Currently, only vtkIdType array is supported.
   * Set to NULL string (default) to use the point ids instead.
   */
  vtkSetStringMacro(PointIdArrayName);
  vtkGetStringMacro(PointIdArrayName);
  vtkSetStringMacro(CellIdArrayName);
  vtkGetStringMacro(CellIdArrayName);
  ///@}

  ///@{
  /**
   * Generally, this class can render the composite id when iterating
   * over composite datasets. However in some cases (as in AMR), the rendered
   * structure may not correspond to the input data, in which case we need
   * to provide a cell array that can be used to render in the composite id in
   * selection passes. Set to NULL (default) to not override the composite id
   * color set by vtkCompositePainter if any.
   * The array *MUST* be a cell array.
   * The array's DataType *MUST* be VTK_UNSIGNED_INT.
   */
  vtkSetStringMacro(CompositeIdArrayName);
  vtkGetStringMacro(CompositeIdArrayName);
  ///@}

  ///@{
  /**
   * If this class should override the process id using a data-array,
   * set this variable to the name of the array to use. It must be a
   * point-array.
   * The array's DataType *MUST* be VTK_UNSIGNED_INT.
   */
  vtkSetStringMacro(ProcessIdArrayName);
  vtkGetStringMacro(ProcessIdArrayName);
  ///@}

  /**
   * Return bounding box (array of six doubles) of data expressed as
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   */
  double* GetBounds() VTK_SIZEHINT(6) override;
  void GetBounds(double bounds[6]) override { this->Superclass::GetBounds(bounds); }

  /**
   * Make a shallow copy of this mapper.
   */
  void ShallowCopy(vtkAbstractMapper* m) override;

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
   * Currently only point data is supported.
   */
  virtual void MapDataArrayToVertexAttribute(const char* vertexAttributeName,
    const char* dataArrayName, int fieldAssociation, int componentno = -1);

  // Specify a data array to use as the texture coordinate
  // for a named texture. See vtkProperty.h for how to
  // name textures.
  virtual void MapDataArrayToMultiTextureAttribute(
    const char* textureName, const char* dataArrayName, int fieldAssociation, int componentno = -1);

  /**
   * Remove a vertex attribute mapping.
   */
  virtual void RemoveVertexAttributeMapping(const char* vertexAttributeName);

  /**
   * Remove all vertex attributes.
   */
  virtual void RemoveAllVertexAttributeMappings();

  /**
   * see vtkAlgorithm for details
   */
  vtkTypeBool ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**\brief Methods for VBO coordinate shift+scale-computation.
   *
   * By default, shift and scale vectors are enabled
   * whenever CreateVBO is called with points whose
   * bounds are many bbox-lengths away from the origin.
   *
   * Shifting and scaling may be completely disabled,
   * or manually specified, or left at the default.
   *
   * Manual specification is for the case when you
   * will be calling AppendVBO instead of just CreateVBO
   * and know better bounds than the what CreateVBO
   * might produce.
   *
   * The automatic method tells CreatVBO to compute shift and
   * scale vectors that remap the points to the unit cube.
   *
   * The camera method will shift scale the VBO so that the visible
   * part of the data has reasonable values.
   */
  enum ShiftScaleMethodType
  {
    DISABLE_SHIFT_SCALE,     //!< Do not shift/scale point coordinates. Ever!
    AUTO_SHIFT_SCALE,        //!< The default, automatic computation.
    ALWAYS_AUTO_SHIFT_SCALE, //!< Always shift scale using auto computed values
    MANUAL_SHIFT_SCALE,      //!< Manual shift/scale (for use with AppendVBO)
    AUTO_SHIFT,              //!< Only Apply the shift
    NEAR_PLANE_SHIFT_SCALE,  //!< Shift scale based on camera settings
    FOCAL_POINT_SHIFT_SCALE  //!< Shift scale based on camera settings
  };

  /**\brief A convenience method for enabling/disabling
   *   the VBO's shift+scale transform.
   */
  virtual void SetVBOShiftScaleMethod(int) {}
  virtual int GetVBOShiftScaleMethod() { return this->ShiftScaleMethod; }

  /**\brief Pause per-render updates to VBO shift+scale parameters.
   *
   * For large datasets, re-uploading the VBO during user interaction
   * can cause stutters in the framerate. Interactors can use this
   * method to force UpdateCameraShiftScale to return immediately
   * (without changes) while users are zooming/rotating/etc. and then
   * re-enable shift-scale just before a still render.
   *
   * This setting has no effect unless the shift-scale method is set
   * to NEAR_PLANE_SHIFT_SCALE or FOCAL_POINT_SHIFT_SCALE.
   *
   * Changing this setting does **not** mark the mapper as modified as
   * that would force a VBO upload – defeating its own purpose.
   */
  virtual void SetPauseShiftScale(bool pauseShiftScale) { this->PauseShiftScale = pauseShiftScale; }
  vtkGetMacro(PauseShiftScale, bool);
  vtkBooleanMacro(PauseShiftScale, bool);

protected:
  vtkPolyDataMapper();
  ~vtkPolyDataMapper() override;

  /**
   * Called in GetBounds(). When this method is called, the consider the input
   * to be updated depending on whether this->Static is set or not. This method
   * simply obtains the bounds from the data-object and returns it.
   */
  virtual void ComputeBounds();

  int Piece;
  int NumberOfPieces;
  int NumberOfSubPieces;
  int GhostLevel;
  bool SeamlessU, SeamlessV;
  int ShiftScaleMethod; // for points
  bool PauseShiftScale;

  // additional picking indirection
  char* PointIdArrayName = nullptr;
  char* CellIdArrayName = nullptr;
  char* CompositeIdArrayName = nullptr;
  char* ProcessIdArrayName = nullptr;

  int FillInputPortInformation(int, vtkInformation*) override;

private:
  vtkPolyDataMapper(const vtkPolyDataMapper&) = delete;
  void operator=(const vtkPolyDataMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
