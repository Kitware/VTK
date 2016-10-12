/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSlice.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageSlice
 * @brief   represents an image in a 3D scene
 *
 * vtkImageSlice is used to represent an image in a 3D scene.  It displays
 * the image either as a slice or as a projection from the camera's
 * perspective. Adjusting the position and orientation of the slice
 * is done by adjusting the focal point and direction of the camera,
 * or alternatively the slice can be set manually in vtkImageMapper3D.
 * The lookup table and window/leve are set in vtkImageProperty.
 * Prop3D methods such as SetPosition() and RotateWXYZ() change the
 * position and orientation of the data with respect to VTK world
 * coordinates.
 * @par Thanks:
 * Thanks to David Gobbi at the Seaman Family MR Centre and Dept. of Clinical
 * Neurosciences, Foothills Medical Centre, Calgary, for providing this class.
 * @sa
 * vtkImageMapper3D vtkImageProperty vtkProp3D
*/

#ifndef vtkImageSlice_h
#define vtkImageSlice_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkProp3D.h"

class vtkRenderer;
class vtkPropCollection;
class vtkImageProperty;
class vtkImageMapper3D;

class VTKRENDERINGCORE_EXPORT vtkImageSlice : public vtkProp3D
{
public:
  vtkTypeMacro(vtkImageSlice,vtkProp3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Creates an Image with the following defaults: origin(0,0,0)
   * position=(0,0,0) scale=1 visibility=1 pickable=1 dragable=1
   * orientation=(0,0,0).
   */
  static vtkImageSlice *New();

  //@{
  /**
   * Set/Get the mapper.
   */
  void SetMapper(vtkImageMapper3D *mapper);
  vtkGetObjectMacro(Mapper, vtkImageMapper3D);
  //@}

  //@{
  /**
   * Set/Get the image display properties.
   */
  void SetProperty(vtkImageProperty *property);
  virtual vtkImageProperty *GetProperty();
  //@}

  /**
   * Update the rendering pipeline by updating the ImageMapper
   */
  void Update();

  //@{
  /**
   * Get the bounds - either all six at once
   * (xmin, xmax, ymin, ymax, zmin, zmax) or one at a time.
   */
  double *GetBounds();
  void GetBounds(double bounds[6]) { this->vtkProp3D::GetBounds( bounds ); };
  double GetMinXBound();
  double GetMaxXBound();
  double GetMinYBound();
  double GetMaxYBound();
  double GetMinZBound();
  double GetMaxZBound();
  //@}

  /**
   * Return the MTime also considering the property etc.
   */
  vtkMTimeType GetMTime();

  /**
   * Return the mtime of anything that would cause the rendered image to
   * appear differently. Usually this involves checking the mtime of the
   * prop plus anything else it depends on such as properties, mappers,
   * etc.
   */
  vtkMTimeType GetRedrawMTime();

  //@{
  /**
   * Force the actor to be treated as translucent.
   */
  vtkGetMacro(ForceTranslucent, bool);
  vtkSetMacro(ForceTranslucent, bool);
  vtkBooleanMacro(ForceTranslucent, bool);
  //@}

  /**
   * Shallow copy of this vtkImageSlice. Overloads the virtual vtkProp method.
   */
  void ShallowCopy(vtkProp *prop);

  /**
   * For some exporters and other other operations we must be
   * able to collect all the actors, volumes, and images. These
   * methods are used in that process.
   */
  void GetImages(vtkPropCollection *);

  //@{
  /**
   * Support the standard render methods.
   */
  virtual int RenderOverlay(vtkViewport *viewport);
  virtual int RenderOpaqueGeometry(vtkViewport *viewport);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport *viewport);
  //@}

  /**
   * Internal method, should only be used by rendering.
   * This method will always return 0 unless ForceTranslucent is On.
   */
  virtual int HasTranslucentPolygonalGeometry();

  /**
   * This causes the image and its mapper to be rendered. Note that a side
   * effect of this method is that the pipeline will be updated.
   */
  virtual void Render(vtkRenderer *);

  /**
   * Release any resources held by this prop.
   */
  void ReleaseGraphicsResources(vtkWindow *win);

  /**
   * For stacked image rendering, set the pass.  The first pass
   * renders just the backing polygon, the second pass renders
   * the image, and the third pass renders the depth buffer.
   * Set to -1 to render all of these in the same pass.
   */
  void SetStackedImagePass(int pass);

protected:
  vtkImageSlice();
  ~vtkImageSlice();

  vtkImageMapper3D *Mapper;
  vtkImageProperty *Property;

  bool ForceTranslucent;

private:
  vtkImageSlice(const vtkImageSlice&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageSlice&) VTK_DELETE_FUNCTION;
};

#endif
