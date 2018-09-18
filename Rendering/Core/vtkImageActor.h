/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageActor
 * @brief   draw an image in a rendered 3D scene
 *
 * vtkImageActor is used to render an image in a 3D scene.  The image
 * is placed at the origin of the image, and its size is controlled by the
 * image dimensions and image spacing. The orientation of the image is
 * orthogonal to one of the x-y-z axes depending on which plane the
 * image is defined in.  This class has been mostly superseded by
 * the vtkImageSlice class, which provides more functionality than
 * vtkImageActor.
 *
 * @sa
 * vtkImageData vtkImageSliceMapper vtkImageProperty
*/

#ifndef vtkImageActor_h
#define vtkImageActor_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkImageSlice.h"

class vtkAlgorithm;
class vtkPropCollection;
class vtkRenderer;
class vtkImageData;


class VTKRENDERINGCORE_EXPORT vtkImageActor : public vtkImageSlice
{
public:
  vtkTypeMacro(vtkImageActor,vtkImageSlice);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Instantiate the image actor.
   */
  static vtkImageActor *New();

  //@{
  /**
   * Set/Get the image data input for the image actor.  This is for
   * backwards compatibility, for a proper pipeline connection you
   * should use GetMapper()->SetInputConnection() instead.
   */
  virtual void SetInputData(vtkImageData *);
  virtual vtkImageData *GetInput();
  //@}

  //@{
  /**
   * Turn on/off linear interpolation of the image when rendering.
   * More options are available in the Property of the image actor.
   */
  virtual void SetInterpolate(vtkTypeBool);
  virtual vtkTypeBool GetInterpolate();
  vtkBooleanMacro(Interpolate,vtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the object's opacity. 1.0 is totally opaque and 0.0 is completely
   * transparent. The default is 1.0.
   */
  virtual void SetOpacity(double);
  virtual double GetOpacity();
  double GetOpacityMinValue() { return 0.0; }
  double GetOpacityMaxValue() { return 1.0; }
  //@}

  //@{
  /**
   * The image extent is generally set explicitly, but if not set
   * it will be determined from the input image data.
   */
  void SetDisplayExtent(const int extent[6]);
  void SetDisplayExtent(int minX, int maxX, int minY, int maxY,
                        int minZ, int maxZ);
  void GetDisplayExtent(int extent[6]);
  int *GetDisplayExtent() VTK_SIZEHINT(6) {return this->DisplayExtent;}
  //@}

  //@{
  /**
   * Get the bounds of this image actor. Either copy the bounds
   * into a user provided array or return a pointer to an array.
   * In either case the bounds is expressed as a 6-vector
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   */
  double *GetBounds() VTK_SIZEHINT(6) override;
  void GetBounds(double bounds[6]) { this->Superclass::GetBounds(bounds); };
  //@}

  //@{
  /**
   * Get the bounds of the data that is displayed by this image
   * actor.  If the transformation matrix for this actor is the
   * identity matrix, this will return the same value as
   * GetBounds.
   */
  double *GetDisplayBounds();
  void GetDisplayBounds(double bounds[6]);
  //@}

  //@{
  /**
   * Return the slice number (& min/max slice number) computed from the display
   * extent.
   */
  int GetSliceNumber();
  int GetSliceNumberMax();
  int GetSliceNumberMin();
  //@}

  //@{
  /**
   * Set/Get the current slice number. The axis Z in ZSlice does not
   * necessarily have any relation to the z axis of the data on disk.
   * It is simply the axis orthogonal to the x,y, display plane.
   * GetWholeZMax and Min are convenience methods for obtaining
   * the number of slices that can be displayed. Again the number
   * of slices is in reference to the display z axis, which is not
   * necessarily the z axis on disk. (due to reformatting etc)
   */
  void SetZSlice(int z) {this->SetDisplayExtent(
    this->DisplayExtent[0], this->DisplayExtent[1],
    this->DisplayExtent[2], this->DisplayExtent[3], z, z);
  };
  int GetZSlice() { return this->DisplayExtent[4];};
  int GetWholeZMin();
  int GetWholeZMax();
  //@}

  /**
   * Internal method, should only be used by rendering.
   * Returns true if this image actor has an alpha component or if it
   * has an opacity that is less than 1.0.  This can be overridden by
   * ForceOpaqueOn(), which forces this method to return false, or
   * ForceTranslucentOn(), which forces this method to return true.
   */
  vtkTypeBool HasTranslucentPolygonalGeometry() override;

  //@{
  /**
   * Force the actor to be rendered during the opaque rendering pass.
   * Default is false.
   * See also: ForceTranslucentOn() to use translucent rendering pass.
   */
  vtkGetMacro(ForceOpaque, bool);
  vtkSetMacro(ForceOpaque, bool);
  vtkBooleanMacro(ForceOpaque, bool);
  //@}

protected:
  vtkImageActor();
  ~vtkImageActor() override;

  /**
   * Guess the orientation from the extent.  The orientation will be Z
   * unless the extent is single-slice in one of the other directions.
   */
  static int GetOrientationFromExtent(const int extent[6]);

  int           DisplayExtent[6];
  double        DisplayBounds[6];

  // Convenience function that returns the input of the mapper
  vtkAlgorithm *GetInputAlgorithm();

  // the result of HasTranslucentPolygonalGeometry is cached
  vtkTimeStamp TranslucentComputationTime;
  int TranslucentCachedResult;
  bool ForceOpaque;

private:
  vtkImageActor(const vtkImageActor&) = delete;
  void operator=(const vtkImageActor&) = delete;
};

#endif
