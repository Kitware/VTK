// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAreaPicker
 * @brief   Picks props behind a selection rectangle on a viewport.
 *
 *
 * The vtkAreaPicker picks all vtkProp3Ds that lie behind the screen space
 * rectangle from x0,y0 and x1,y1. The selection is based upon the bounding
 * box of the prop and is thus not exact.
 *
 * Like vtkPicker, a pick results in a list of Prop3Ds because many props may
 * lie within the pick frustum. You can also get an AssemblyPath, which in this
 * case is defined to be the path to the one particular prop in the Prop3D list
 * that lies nearest to the near plane.
 *
 * This picker also returns the selection frustum, defined as either a
 * vtkPlanes, or a set of eight corner vertices in world space. The vtkPlanes
 * version is an ImplicitFunction, which is suitable for use with the
 * vtkExtractGeometry. The six frustum planes are in order: left, right,
 * bottom, top, near, far
 *
 * Because this picker picks everything within a volume, the world pick point
 * result is ill-defined. Therefore if you ask this class for the world pick
 * position, you will get the centroid of the pick frustum. This may be outside
 * of all props in the prop list.
 *
 * @sa
 * vtkInteractorStyleRubberBandPick, vtkExtractSelectedFrustum.
 */

#ifndef vtkAreaPicker_h
#define vtkAreaPicker_h

#include "vtkAbstractPropPicker.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObject;
class vtkRenderer;
class vtkPoints;
class vtkPlanes;
class vtkProp3DCollection;
class vtkAbstractMapper3D;
class vtkDataSet;
class vtkExtractSelectedFrustum;
class vtkProp;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkAreaPicker : public vtkAbstractPropPicker
{
public:
  static vtkAreaPicker* New();
  vtkTypeMacro(vtkAreaPicker, vtkAbstractPropPicker);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the default screen rectangle to pick in.
   */
  void SetPickCoords(double x0, double y0, double x1, double y1);

  /**
   * Set the default renderer to pick on.
   */
  void SetRenderer(vtkRenderer*);

  /**
   * Perform an AreaPick within the default screen rectangle and renderer.
   */
  virtual int Pick();

  /**
   * Perform pick operation in volume behind the given screen coordinates.
   * Props intersecting the selection frustum will be accessible via GetProp3D.
   * GetPlanes returns a vtkImplicitFunction suitable for vtkExtractGeometry.
   */
  virtual int AreaPick(double x0, double y0, double x1, double y1, vtkRenderer* renderer = nullptr);

  /**
   * Perform pick operation in volume behind the given screen coordinate.
   * This makes a thin frustum around the selected pixel.
   * Note: this ignores Z in order to pick everying in a volume from z=0 to z=1.
   */
  int Pick(double x0, double y0, double vtkNotUsed(z0), vtkRenderer* renderer = nullptr) override
  {
    return this->AreaPick(x0, y0, x0 + 1.0, y0 + 1.0, renderer);
  }

  ///@{
  /**
   * Return mapper that was picked (if any).
   */
  vtkGetObjectMacro(Mapper, vtkAbstractMapper3D);
  ///@}

  ///@{
  /**
   * Get a pointer to the dataset that was picked (if any). If nothing
   * was picked then NULL is returned.
   */
  vtkGetObjectMacro(DataSet, vtkDataSet);
  ///@}

  ///@{
  /**
   * Get a pointer to the dataobject that was picked (if any). If nothing
   * was picked then NULL is returned.
   */
  vtkGetObjectMacro(DataObject, vtkDataObject);
  ///@}

  /**
   * Return a collection of all the prop 3D's that were intersected
   * by the pick ray. This collection is not sorted.
   */
  vtkProp3DCollection* GetProp3Ds() { return this->Prop3Ds; }

  ///@{
  /**
   * Return the six planes that define the selection frustum. The implicit
   * function defined by the planes evaluates to negative inside and positive
   * outside.
   */
  vtkGetObjectMacro(Frustum, vtkPlanes);
  ///@}

  ///@{
  /**
   * Return eight points that define the selection frustum.
   */
  vtkGetObjectMacro(ClipPoints, vtkPoints);
  ///@}

protected:
  vtkAreaPicker();
  ~vtkAreaPicker() override;

  void Initialize() override;
  void DefineFrustum(double x0, double y0, double x1, double y1, vtkRenderer*);
  virtual int PickProps(vtkRenderer* renderer);
  int TypeDecipher(vtkProp*, vtkAbstractMapper3D**);

  int ABoxFrustumIsect(double bounds[], double& mindist);

  vtkPoints* ClipPoints;
  vtkPlanes* Frustum;

  vtkProp3DCollection* Prop3Ds; // candidate actors (based on bounding box)
  vtkAbstractMapper3D* Mapper;  // selected mapper (if the prop has a mapper)
  vtkDataSet* DataSet;          // selected dataset (if there is one)
  vtkDataObject* DataObject;    // selected dataobject (this is useful to represent selected objects
                                // which directly derive vtkDataObject)

  // used internally to do prop intersection tests
  vtkExtractSelectedFrustum* FrustumExtractor;

  double X0;
  double Y0;
  double X1;
  double Y1;

private:
  vtkAreaPicker(const vtkAreaPicker&) = delete;
  void operator=(const vtkAreaPicker&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
