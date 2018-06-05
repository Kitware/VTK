/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellPicker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCellPicker
 * @brief   ray-cast cell picker for all kinds of Prop3Ds
 *
 * vtkCellPicker will shoot a ray into a 3D scene and return information
 * about the first object that the ray hits.  It works for all Prop3Ds.
 * For vtkVolume objects, it shoots a ray into the volume and returns
 * the point where the ray intersects an isosurface of a chosen opacity.
 * For vtkImage objects, it intersects the ray with the displayed
 * slice. For vtkActor objects, it intersects the actor's polygons.
 * If the object's mapper has ClippingPlanes, then it takes the clipping
 * into account, and will return the Id of the clipping plane that was
 * intersected.
 * For all prop types, it returns point and cell information, plus the
 * normal of the surface that was intersected at the pick position.  For
 * volumes and images, it also returns (i,j,k) coordinates for the point
 * and the cell that were picked.
 *
 * @sa
 * vtkPicker vtkPointPicker vtkVolumePicker
 *
 * @par Thanks:
 * This class was contributed to VTK by David Gobbi on behalf of Atamai Inc.,
 * as an enhancement to the original vtkCellPicker.
*/

#ifndef vtkCellPicker_h
#define vtkCellPicker_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkPicker.h"

class vtkMapper;
class vtkTexture;
class vtkAbstractVolumeMapper;
class vtkImageMapper3D;
class vtkPlaneCollection;
class vtkPiecewiseFunction;
class vtkDataArray;
class vtkDoubleArray;
class vtkIdList;
class vtkCell;
class vtkGenericCell;
class vtkImageData;
class vtkAbstractCellLocator;
class vtkCollection;
class vtkMatrix4x4;

class VTKRENDERINGCORE_EXPORT vtkCellPicker : public vtkPicker
{
public:
  static vtkCellPicker *New();
  vtkTypeMacro(vtkCellPicker, vtkPicker);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform pick operation with selection point provided. Normally the
   * first two values are the (x,y) pixel coordinates for the pick, and
   * the third value is z=0. The return value will be non-zero if
   * something was successfully picked.
   */
  int Pick(double selectionX, double selectionY, double selectionZ,
                   vtkRenderer *renderer) override;

  /**
   * Perform pick operation with selection point provided. The
   * selectionPt is in world coordinates.
   * Return non-zero if something was successfully picked.
   */
  int Pick3DRay(double selectionPt[3], double orient[4], vtkRenderer *ren) override;

  /**
   * Add a locator for one of the data sets that will be included in the
   * scene.  You must set up the locator with exactly the same data set
   * that was input to the mapper of one or more of the actors in the
   * scene.  As well, you must either build the locator before doing the
   * pick, or you must turn on LazyEvaluation in the locator to make it
   * build itself on the first pick.  Note that if you try to add the
   * same locator to the picker twice, the second addition will be ignored.
   */
  void AddLocator(vtkAbstractCellLocator *locator);

  /**
   * Remove a locator that was previously added.  If you try to remove a
   * nonexistent locator, then nothing will happen and no errors will be
   * raised.
   */
  void RemoveLocator(vtkAbstractCellLocator *locator);

  /**
   * Remove all locators associated with this picker.
   */
  void RemoveAllLocators();

  //@{
  /**
   * Set the opacity isovalue to use for defining volume surfaces.  The
   * pick will occur at the location along the pick ray where the
   * opacity of the volume is equal to this isovalue.  If you want to do
   * the pick based on an actual data isovalue rather than the opacity,
   * then pass the data value through the scalar opacity function before
   * using this method.
   */
  vtkSetMacro(VolumeOpacityIsovalue, double);
  vtkGetMacro(VolumeOpacityIsovalue, double);
  //@}

  //@{
  /**
   * Use the product of the scalar and gradient opacity functions when
   * computing the opacity isovalue, instead of just using the scalar
   * opacity. This parameter is only relevant to volume picking and
   * is off by default.
   */
  vtkSetMacro(UseVolumeGradientOpacity, vtkTypeBool);
  vtkBooleanMacro(UseVolumeGradientOpacity, vtkTypeBool);
  vtkGetMacro(UseVolumeGradientOpacity, vtkTypeBool);
  //@}

  //@{
  /**
   * The PickClippingPlanes setting controls how clipping planes are
   * handled by the pick.  If it is On, then the clipping planes become
   * pickable objects, even though they are usually invisible.  This
   * means that if the pick ray intersects a clipping plane before it
   * hits anything else, the pick will stop at that clipping plane.
   * The GetProp3D() and GetMapper() methods will return the Prop3D
   * and Mapper that the clipping plane belongs to.  The
   * GetClippingPlaneId() method will return the index of the clipping
   * plane so that you can retrieve it from the mapper, or -1 if no
   * clipping plane was picked.
   */
  vtkSetMacro(PickClippingPlanes, vtkTypeBool);
  vtkBooleanMacro(PickClippingPlanes, vtkTypeBool);
  vtkGetMacro(PickClippingPlanes, vtkTypeBool);
  //@}

  //@{
  /**
   * Get the index of the clipping plane that was intersected during
   * the pick.  This will be set regardless of whether PickClippingPlanes
   * is On, all that is required is that the pick intersected a clipping
   * plane of the Prop3D that was picked.  The result will be -1 if the
   * Prop3D that was picked has no clipping planes, or if the ray didn't
   * intersect the planes.
   */
  vtkGetMacro(ClippingPlaneId, int);
  //@}

  //@{
  /**
   * Return the normal of the picked surface at the PickPosition.  If no
   * surface was picked, then a vector pointing back at the camera is
   * returned.
   */
  vtkGetVectorMacro(PickNormal, double, 3);
  //@}

  //@{
  /**
   * Return the normal of the surface at the PickPosition in mapper
   * coordinates.  The result is undefined if no prop was picked.
   */
  vtkGetVector3Macro(MapperNormal, double);
  //@}

  //@{
  /**
   * Get the structured coordinates of the point at the PickPosition.
   * Only valid for image actors and volumes with vtkImageData.
   */
  vtkGetVector3Macro(PointIJK, int);
  //@}

  //@{
  /**
   * Get the structured coordinates of the cell at the PickPosition.
   * Only valid for image actors and volumes with vtkImageData.
   * Combine this with the PCoords to get the position within the cell.
   */
  vtkGetVector3Macro(CellIJK, int);
  //@}

  //@{
  /**
   * Get the id of the picked point. If PointId = -1, nothing was picked.
   * This point will be a member of any cell that is picked.
   */
  vtkGetMacro(PointId, vtkIdType);
  //@}

  //@{
  /**
   * Get the id of the picked cell. If CellId = -1, nothing was picked.
   */
  vtkGetMacro(CellId, vtkIdType);
  //@}

  //@{
  /**
   * Get the subId of the picked cell. This is useful, for example, if
   * the data is made of triangle strips. If SubId = -1, nothing was picked.
   */
  vtkGetMacro(SubId, int);
  //@}

  //@{
  /**
   * Get the parametric coordinates of the picked cell. Only valid if
   * a prop was picked.  The PCoords can be used to compute the weights
   * that are needed to interpolate data values within the cell.
   */
  vtkGetVector3Macro(PCoords, double);
  //@}

  /**
   * Get the texture that was picked.  This will always be set if the
   * picked prop has a texture, and will always be null otherwise.
   */
  vtkTexture *GetTexture() { return this->Texture; };

  //@{
  /**
   * If this is "On" and if the picked prop has a texture, then the data
   * returned by GetDataSet() will be the texture's data instead of the
   * mapper's data.  The GetPointId(), GetCellId(), GetPCoords() etc. will
   * all return information for use with the texture's data.  If the picked
   * prop does not have any texture, then GetDataSet() will return the
   * mapper's data instead and GetPointId() etc. will return information
   * related to the mapper's data.  The default value of PickTextureData
   * is "Off".
   */
  vtkSetMacro(PickTextureData, vtkTypeBool);
  vtkBooleanMacro(PickTextureData, vtkTypeBool);
  vtkGetMacro(PickTextureData, vtkTypeBool);
  //@}

protected:
  vtkCellPicker();
  ~vtkCellPicker() override;

  void Initialize() override;

  virtual void ResetPickInfo();

  double IntersectWithLine(const double p1[3], const double p2[3], double tol,
                                  vtkAssemblyPath *path, vtkProp3D *p,
                                  vtkAbstractMapper3D *m) override;

  virtual double IntersectActorWithLine(const double p1[3], const double p2[3],
                                        double t1, double t2, double tol,
                                        vtkProp3D *prop, vtkMapper *mapper);

  virtual bool IntersectDataSetWithLine(vtkDataSet* dataSet,
                                        const double p1[3], const double p2[3],
                                        double t1, double t2, double tol,
                                        vtkAbstractCellLocator* &locator,
                                        vtkIdType& cellId, int& subId,
                                        double &tMin, double &pDistMin,
                                        double xyz[3], double minPCoords[3] );

  virtual double IntersectVolumeWithLine(const double p1[3],
                                         const double p2[3],
                                         double t1, double t2,
                                         vtkProp3D *prop,
                                         vtkAbstractVolumeMapper *mapper);

  virtual double IntersectImageWithLine(const double p1[3],
                                        const double p2[3],
                                        double t1, double t2,
                                        vtkProp3D *prop,
                                        vtkImageMapper3D *mapper);

  virtual double IntersectProp3DWithLine(const double p1[3],
                                         const double p2[3],
                                         double t1, double t2, double tol,
                                         vtkProp3D *prop,
                                         vtkAbstractMapper3D *mapper);

  static int ClipLineWithPlanes(vtkAbstractMapper3D *mapper,
                                vtkMatrix4x4 *propMatrix,
                                const double p1[3], const double p2[3],
                                double &t1, double &t2, int& planeId);

  static int ClipLineWithExtent(const int extent[6],
                                const double x1[3], const double x2[3],
                                double &t1, double &t2, int &planeId);

  static int ComputeSurfaceNormal(vtkDataSet *data, vtkCell *cell,
                                  const double *weights, double normal[3]);

  static int ComputeSurfaceTCoord(vtkDataSet *data, vtkCell *cell,
                                  const double *weights, double tcoord[3]);

  static int HasSubCells(int cellType);

  static int GetNumberOfSubCells(vtkIdList *pointIds, int cellType);

  static void GetSubCell(vtkDataSet *data, vtkIdList *pointIds, int subId,
                         int cellType, vtkGenericCell *cell);

  static void SubCellFromCell(vtkGenericCell *cell, int subId);

  void SetImageDataPickInfo(const double x[3], const int extent[6]);

  double ComputeVolumeOpacity(const int xi[3], const double pcoords[3],
                              vtkImageData *data, vtkDataArray *scalars,
                              vtkPiecewiseFunction *scalarOpacity,
                              vtkPiecewiseFunction *gradientOpacity);

  vtkCollection *Locators;

  double VolumeOpacityIsovalue;
  vtkTypeBool UseVolumeGradientOpacity;
  vtkTypeBool PickClippingPlanes;
  int ClippingPlaneId;

  vtkIdType PointId;
  vtkIdType CellId;
  int SubId;
  double PCoords[3];

  int PointIJK[3];
  int CellIJK[3];

  double PickNormal[3];
  double MapperNormal[3];

  vtkTexture *Texture;
  vtkTypeBool PickTextureData;

private:
  void ResetCellPickerInfo();

  vtkGenericCell *Cell; //used to accelerate picking
  vtkIdList *PointIds; // used to accelerate picking
  vtkDoubleArray *Gradients; //used in volume picking

private:
  vtkCellPicker(const vtkCellPicker&) = delete;
  void operator=(const vtkCellPicker&) = delete;
};

#endif


