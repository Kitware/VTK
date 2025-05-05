// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHardwarePicker
 * @brief   pick a point or snap to point of an actor/prop using graphics hardware
 *
 * vtkHardwarePicker is used to pick point or snap to point of an actor/prop given a selection
 * point (in display coordinates) and a renderer. This class uses graphics hardware/rendering
 * system to pick rapidly (as compared to using ray casting as does vtkCellPicker and
 * vtkPointPicker). This class determines the actor/prop pick position, and pick normal in world
 * coordinates; pointId is determined if snapping is enabled, otherwise the cellId is determined.
 * if no actor/prop is picked, pick position = camera focal point, and pick normal = camera plane
 * normal.
 *
 * @warning This class supports only picking in a screen, and not in VR.
 *
 * @sa
 * vtkPropPicker vtkPicker vtkWorldPointPicker vtkCellPicker vtkPointPicker
 */

#ifndef vtkHardwarePicker_h
#define vtkHardwarePicker_h

#include "vtkAbstractPropPicker.h"
#include "vtkNew.h"                 // For vtkNew
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkSmartPointer.h"        // For vtkSmartPointer
#include "vtkStringToken.h"         // for vtkStringToken
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractMapper3D;
class vtkCell;
class vtkCompositeDataSet;
class vtkDataObject;
class vtkDataSet;
class vtkSelection;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkHardwarePicker : public vtkAbstractPropPicker
{
public:
  static vtkHardwarePicker* New();
  vtkTypeMacro(vtkHardwarePicker, vtkAbstractPropPicker);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get if the picker will snap to the closest mesh point or get the actual intersected point.
   * Default is off.
   */
  vtkSetMacro(SnapToMeshPoint, bool);
  vtkGetMacro(SnapToMeshPoint, bool);
  vtkBooleanMacro(SnapToMeshPoint, bool);
  ///@}

  ///@{
  /**
   * When SnapToMeshPoint is on, this is the pixel tolerance to use when snapping.
   * Default is 5.
   */
  vtkSetMacro(PixelTolerance, int);
  vtkGetMacro(PixelTolerance, int);
  ///@}

  ///@{
  /**
   * Return mapper that was picked (if any).
   *
   * Note: Use vtkWeakPointer. This is because the Mapper may be deleted.
   */
  vtkGetObjectMacro(Mapper, vtkAbstractMapper3D);
  ///@}

  ///@{
  /**
   * Get a pointer to the dataset that was picked (if any). If nothing
   * was picked then nullptr is returned.
   *
   * Note: Use vtkWeakPointer. This is because the DataSet may be deleted.
   */
  vtkGetObjectMacro(DataSet, vtkDataSet);
  ///@}

  ///@{
  /**
   * Get a pointer to the dataobject that was picked (if any). If nothing
   * was picked then nullptr is returned.
   *
   * Note: Use vtkWeakPointer. This is because the DataObject may be deleted.
   */
  vtkGetObjectMacro(DataObject, vtkDataObject);
  ///@}

  ///@{
  /**
   * Get a pointer to the composite dataset that was picked (if any). If nothing
   * was picked or a non-composite data object was picked then nullptr is returned.
   *
   * Note: Use vtkWeakPointer. This is because the CompositeDataSet may be deleted.
   */
  vtkGetObjectMacro(CompositeDataSet, vtkCompositeDataSet);
  ///@}

  ///@{
  /**
   * Get the flat block index of the vtkDataSet in the composite dataset
   * that was picked (if any). If nothing was picked or a non-composite
   * data object was picked then -1 is returned.
   */
  vtkGetMacro(FlatBlockIndex, vtkIdType);
  ///@}

  ///@{
  /**
   * Get the id of the picked point.
   *
   * If a prop is picked:
   *
   * 1) if SnapOnMeshPoint is on, the pointId of the prop's dataset will be returned
   * 2) If SnapOnMeshPoint is off, PointId = -1;
   *
   * If a prop is not picked, PointId = -1;
   */
  vtkGetMacro(PointId, vtkIdType);
  ///@}

  ///@{
  /**
   * Get the id of the picked cell.
   *
   * If a prop is picked:
   *
   * 1) If SnapOnMeshPoint is on, CellId = -1.
   * 2) If SnapOnMeshPoint is off, the cellId of the prop's dataset will be returned
   *
   * if a prop is not picked, CellId = -1.
   */
  vtkGetMacro(CellId, vtkIdType);
  ///@}

  ///@{
  /**
   * Get the subId of the picked cell. This is useful, for example, if
   * the data is made of triangle strips.
   *
   * If a prop is picked:
   *
   * 1) If SnapOnMeshPoint is on, SubId = -1.
   * 2) If SnapOnMeshPoint is off and the picked cell is a triangle strip, the subId of the
   * intersected triangle will be returned, otherwise SubId = -1.
   *
   * If a prop is not picked, SubId = -1.
   */
  vtkGetMacro(SubId, int);
  ///@}

  ///@{
  /**
   * Get the id of the picked cell type in a vtkCellGrid
   *
   * If a prop is picked, cellGridCellTypeId >= 0
   *
   * if a prop is not picked, CellGridCellTypeId = -1.
   */
  vtkGetMacro(CellGridCellTypeId, vtkIdType);
  ///@}

  ///@{
  /**
   * Get the id of the picked cell/side spec (the vtkDGCell::Source object) in a vtkCellGrid's cell
   * type.
   *
   * If a prop is picked:
   *
   * 1) If a cell spec was picked, CellGridSourceSpecId = 0.
   * 2) If a side spec was picked, the CellGridSourceSpecId will be >= 1.
   *
   * if a prop is not picked, CellGridSourceSpecId = -1.
   */
  vtkGetMacro(CellGridSourceSpecId, vtkIdType);
  ///@}

  ///@{
  /**
   * Get the id of the tuple in the cell/side's connectivity array in a vtkCellGrid
   *
   * If a prop is picked, CellGridTupleId >= 0
   *
   * if a prop is not picked, CellGridTupleId = -1.
   */
  vtkGetMacro(CellGridTupleId, vtkIdType);
  ///@}

  ///@{
  /**
   * Get the parametric coordinates of the picked cell. PCoords can be used to compute the
   * weights that are needed to interpolate data values within the cell.
   *
   * If a prop is picked:
   *
   * 1) If SnapOnMeshPoint is on, PCoords will be a vector of
   * std::numeric_limits<double>::quiet_NaN().
   * 2) If SnapOnMeshPoint is off, PCoords will be extracted and the intersection point of the cell.
   *
   * if a prop is not picked, PCoords will be a vector of std::numeric_limits<double>::quiet_NaN().
   */
  vtkGetVector3Macro(PCoords, double);
  ///@}

  ///@{
  /**
   * Get the normal of the point at the PickPosition.
   *
   * If a prop is picked:
   *
   * 1) If SnapOnMeshPoint is on, the picked normal will be extracted from the PointData normals, if
   * they exist, otherwise a vector of std::numeric_limits<double>::quiet_NaN() will be returned.
   * 2) If SnapOnMeshPoint is off, the picked normal on the intersected cell will be extracted using
   * ray intersection, if the ray intersections was successful, otherwise a vector of
   * std::numeric_limits<double>::quiet_NaN() will be returned.
   *
   * if a prop is not picked, the camera plane normal will be returned will be returned.
   */
  vtkGetVectorMacro(PickNormal, double, 3);
  ///@}

  /**
   * Get if normal is flipped.
   *
   * The normal will be flipped if point normals don't exist and the angle between the PickedNormal
   * and the camera plane normal is more than pi / 2.
   */
  vtkGetMacro(NormalFlipped, bool);

  /**
   * Perform the pick operation set the PickedProp.
   *
   * If something is picked, 1 is returned, and PickPosition, PickNormal, and the rest of the
   * results variables) are extracted from intersection with the PickedProp.
   *
   * If something is not picked, 0 is returned, and PickPosition and PickNormal are extracted from
   * the camera's focal plane.
   */
  int Pick(double selectionX, double selectionY, double selectionZ, vtkRenderer* renderer) override;

protected:
  vtkHardwarePicker();
  ~vtkHardwarePicker() override;

  void Initialize() override;
  // converts the propCandidate into a vtkAbstractMapper3D and returns its pickability
  int TypeDecipher(vtkProp*, vtkAbstractMapper3D**);

  /**
   * Fix normal sign in case the orientation of the picked cell is wrong.
   * When you cast a ray to a 3d object from a specific view angle (camera normal), the angle
   * between the camera normal and the normal of the cell surface that the ray intersected,
   * can have an angle up to pi / 2. This is true because you can't see surface objects
   * with a greater angle than pi / 2, therefore, you can't pick them. In case an angle greater
   * than pi / 2 is computed, we flip the picked normal.
   */
  void FixNormalSign();

  /**
   * Compute the intersection normal either by interpolating the point normals at the
   * intersected point, or by computing the plane normal for the 2D intersected face/cell.
   */
  int ComputeSurfaceNormal(vtkDataSet* data, vtkCell* cell, double* weights);

  /**
   * Compute the intersection using provided dataset
   */
  void ComputeIntersectionFromDataSet(vtkDataSet* ds);

  bool SnapToMeshPoint; // if true, the pick position is snapped to the closest point on the mesh
  int PixelTolerance;   // tolerance for picking when snapping the closest point on the mesh

  vtkNew<vtkPropCollection> PickableProps;         // list of pickable props
  vtkSmartPointer<vtkSelection> HardwareSelection; // result of the hardware selector

  double NearRayPoint[3]; // near ray point
  double FarRayPoint[3];  // far ray point

  vtkAbstractMapper3D* Mapper; // selected mapper (if the prop has a mapper)
  vtkDataSet* DataSet;         // selected dataset (if there is one)
  vtkDataObject* DataObject;   // selected dataobject (useful when the picked object is directly
                               // derived from vtkDataObject)
  vtkCompositeDataSet* CompositeDataSet; // selected dataset (if there is one)
  vtkIdType FlatBlockIndex;              // flat block index, for a composite data set

  vtkIdType PointId;              // id of the picked point
  vtkIdType CellId;               // id of the picked cell
  int SubId;                      // sub id of the picked cell
  vtkIdType CellGridCellTypeId;   // id of the picked cell type in a vtkCellGrid
  vtkIdType CellGridSourceSpecId; // id of the picked cell/side spec in a vtkCellGrid. 0 is for cell
                                  // spec, 1+ is for side specs
  vtkIdType
    CellGridTupleId;    // id of the picked tuple in the cell/side connectivity of a vtkCellGrid
  double PCoords[3];    // parametric coordinates of the picked point
  double PickNormal[3]; // normal of the picked surface
  bool NormalFlipped;   // Flag to indicate if the normal has been flipped

private:
  vtkHardwarePicker(const vtkHardwarePicker&) = delete;
  void operator=(const vtkHardwarePicker&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
