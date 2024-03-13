// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAbstractMapper
 * @brief   abstract class specifies interface to map data
 *
 * vtkAbstractMapper is an abstract class to specify interface between data and
 * graphics primitives or software rendering techniques. Subclasses of
 * vtkAbstractMapper can be used for rendering 2D data, geometry, or volumetric
 * data.
 *
 * @sa
 * vtkAbstractMapper3D vtkMapper vtkPolyDataMapper vtkVolumeMapper
 */

#ifndef vtkAbstractMapper_h
#define vtkAbstractMapper_h

#include "vtkAlgorithm.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALMANUAL

#define VTK_SCALAR_MODE_DEFAULT 0
#define VTK_SCALAR_MODE_USE_POINT_DATA 1
#define VTK_SCALAR_MODE_USE_CELL_DATA 2
#define VTK_SCALAR_MODE_USE_POINT_FIELD_DATA 3
#define VTK_SCALAR_MODE_USE_CELL_FIELD_DATA 4
#define VTK_SCALAR_MODE_USE_FIELD_DATA 5

#define VTK_GET_ARRAY_BY_ID 0
#define VTK_GET_ARRAY_BY_NAME 1

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;
class vtkDataSet;
class vtkPlane;
class vtkPlaneCollection;
class vtkPlanes;
class vtkTimerLog;
class vtkUnsignedCharArray;
class vtkWindow;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALMANUAL vtkAbstractMapper : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkAbstractMapper, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Override Modifiedtime as we have added Clipping planes
   */
  vtkMTimeType GetMTime() override;

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(vtkWindow*) {}

  ///@{
  /**
   * Get the time required to draw the geometry last time it was rendered
   */
  vtkGetMacro(TimeToDraw, double);
  ///@}

  ///@{
  /**
   * Specify clipping planes to be applied when the data is mapped
   * (at most 6 clipping planes can be specified).
   */
  void AddClippingPlane(vtkPlane* plane);
  void RemoveClippingPlane(vtkPlane* plane);
  void RemoveAllClippingPlanes();
  ///@}

  ///@{
  /**
   * Get/Set the vtkPlaneCollection which specifies the
   * clipping planes.
   */
  virtual void SetClippingPlanes(vtkPlaneCollection*);
  vtkGetObjectMacro(ClippingPlanes, vtkPlaneCollection);
  ///@}

  /**
   * An alternative way to set clipping planes: use up to six planes found
   * in the supplied instance of the implicit function vtkPlanes.
   */
  void SetClippingPlanes(vtkPlanes* planes);

  /**
   * Make a shallow copy of this mapper.
   */
  virtual void ShallowCopy(vtkAbstractMapper* m);

  /**
   * Internal helper function for getting the active scalars. The scalar
   * mode indicates where the scalars come from.  The cellFlag is a
   * return value that is set when the scalars actually are cell scalars.
   * (0 for point scalars, 1 for cell scalars, 2 for field scalars)
   * The arrayAccessMode is used to indicate how to retrieve the scalars from
   * field data, per id or per name (if the scalarMode indicates that).
   */
  static vtkDataArray* GetScalars(vtkDataSet* input, int scalarMode, int arrayAccessMode,
    int arrayId, const char* arrayName, int& cellFlag);

  /**
   * Internal helper function for getting the active scalars as an
   * abstract array. The scalar mode indicates where the scalars come
   * from.  The cellFlag is a return value that is set when the
   * scalars actually are cell scalars.  (0 for point scalars, 1 for
   * cell scalars, 2 for field scalars) The arrayAccessMode is used to
   * indicate how to retrieve the scalars from field data, per id or
   * per name (if the scalarMode indicates that).
   */
  static vtkAbstractArray* GetAbstractScalars(vtkDataSet* input, int scalarMode,
    int arrayAccessMode, int arrayId, const char* arrayName, int& cellFlag);

  /**
   * Returns the ghost array associated with the corresponding scalar mode, if present.
   * If no ghost array is available, this method returns `nullptr`. `ghostsToSkip` is an output,
   * and is set to the bit mask associated with the ghost array in the `vtkFieldData` in which
   * the ghost array lives. This bit mask can be ignored if `nullptr` is returned.
   *
   * @sa
   * vtkFieldData
   * vtkDataSetAttributes
   * vtkCellData
   * vtkPointData
   */
  static vtkUnsignedCharArray* GetGhostArray(
    vtkDataSet* input, int scalarMode, unsigned char& ghostsToSkip);

  /**
   * Get the number of clipping planes.
   */
  int GetNumberOfClippingPlanes();

protected:
  vtkAbstractMapper();
  ~vtkAbstractMapper() override;

  vtkTimerLog* Timer;
  double TimeToDraw;
  vtkWindow* LastWindow; // Window used for the previous render
  vtkPlaneCollection* ClippingPlanes;

private:
  vtkAbstractMapper(const vtkAbstractMapper&) = delete;
  void operator=(const vtkAbstractMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
