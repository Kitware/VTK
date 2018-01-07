/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMapper3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageMapper3D
 * @brief   abstract class for mapping images to the screen
 *
 * vtkImageMapper3D is a mapper that will draw a 2D image, or a slice
 * of a 3D image.  The slice plane can be set automatically follow the
 * camera, so that it slices through the focal point and faces the camera.
 * @par Thanks:
 * Thanks to David Gobbi at the Seaman Family MR Centre and Dept. of Clinical
 * Neurosciences, Foothills Medical Centre, Calgary, for providing this class.
 * @sa
 * vtkImage vtkImageProperty vtkImageResliceMapper vtkImageSliceMapper
*/

#ifndef vtkImageMapper3D_h
#define vtkImageMapper3D_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkAbstractMapper3D.h"

class vtkRenderer;
class vtkProp3D;
class vtkPoints;
class vtkMatrix4x4;
class vtkLookupTable;
class vtkScalarsToColors;
class vtkImageSlice;
class vtkImageProperty;
class vtkImageData;
class vtkMultiThreader;
class vtkImageToImageMapper3DFriendship;

class VTKRENDERINGCORE_EXPORT vtkImageMapper3D : public vtkAbstractMapper3D
{
public:
  vtkTypeMacro(vtkImageMapper3D, vtkAbstractMapper3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * This should only be called by the renderer.
   */
  virtual void Render(vtkRenderer *renderer, vtkImageSlice *prop) = 0;

  /**
   * Release any graphics resources that are being consumed by
   * this mapper.  The parameter window is used to determine
   * which graphic resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *) override = 0;

  //@{
  /**
   * The input data for this mapper.
   */
  void SetInputData(vtkImageData *input);
  vtkImageData *GetInput();
  vtkDataSet *GetDataSetInput();
  vtkDataObject *GetDataObjectInput();
  //@}

  //@{
  /**
   * Instead of displaying the image only out to the image
   * bounds, include a half-voxel border around the image.
   * Within this border, the image values will be extrapolated
   * rather than interpolated.
   */
  vtkSetMacro(Border, vtkTypeBool);
  vtkBooleanMacro(Border, vtkTypeBool);
  vtkGetMacro(Border, vtkTypeBool);
  //@}

  //@{
  /**
   * Instead of rendering only to the image border, render out
   * to the viewport boundary with the background color.  The
   * background color will be the lowest color on the lookup
   * table that is being used for the image.
   */
  vtkSetMacro(Background, vtkTypeBool);
  vtkBooleanMacro(Background, vtkTypeBool);
  vtkGetMacro(Background, vtkTypeBool);
  //@}

  //@{
  /**
   * Automatically set the slice position to the camera focal point.
   * This provides a convenient way to interact with the image, since
   * most Interactors directly control the camera.
   */
  vtkSetMacro(SliceAtFocalPoint, vtkTypeBool);
  vtkBooleanMacro(SliceAtFocalPoint, vtkTypeBool);
  vtkGetMacro(SliceAtFocalPoint, vtkTypeBool);
  //@}

  //@{
  /**
   * Automatically set the slice orientation so that it faces the camera.
   * This provides a convenient way to interact with the image, since
   * most Interactors directly control the camera.
   */
  vtkSetMacro(SliceFacesCamera, vtkTypeBool);
  vtkBooleanMacro(SliceFacesCamera, vtkTypeBool);
  vtkGetMacro(SliceFacesCamera, vtkTypeBool);
  //@}

  //@{
  /**
   * A plane that describes what slice of the input is being
   * rendered by the mapper.  This plane is in world coordinates,
   * not data coordinates.  Before using this plane, call Update
   * or UpdateInformation to make sure the plane is up-to-date.
   * These methods are automatically called by Render.
   */
  vtkGetObjectMacro(SlicePlane, vtkPlane);
  //@}

  /**
   * Get the plane as a homogeneous 4-vector that gives the plane
   * equation coefficients.  The prop3D matrix must be provided so
   * that the plane can be converted to data coords.
   */
  virtual void GetSlicePlaneInDataCoords(vtkMatrix4x4 *propMatrix,
                                         double plane[4]);

  //@{
  /**
   * The number of threads to create when rendering.
   */
  vtkSetClampMacro(NumberOfThreads, int, 1, VTK_MAX_THREADS);
  vtkGetMacro(NumberOfThreads, int);
  //@}

  //@{
  /**
   * Turn on streaming, to pull the minimum amount of data from the input.
   * Streaming decreases the memory required to display large images, since
   * only one slice will be pulled through the input pipeline if only
   * one slice is mapped to the screen.  The default behavior is to pull
   * the full 3D input extent through the input pipeline, but to do this
   * only when the input data changes.  The default behavior results in
   * much faster follow-up renders when the input data is static.
   */
  vtkSetMacro(Streaming, vtkTypeBool);
  vtkGetMacro(Streaming, vtkTypeBool);
  vtkBooleanMacro(Streaming, vtkTypeBool);
  //@}

protected:
  vtkImageMapper3D();
  ~vtkImageMapper3D() override;

  //@{
  /**
   * See algorithm for more info
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;
  //@}

  /**
   * Handle requests from the pipeline executive.
   */
  int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inInfo,
                             vtkInformationVector* outInfo) override;

  /**
   * Checkerboard the alpha component of an RGBA image.  The origin and
   * spacing are in pixel units.
   */
  static void CheckerboardRGBA(
    unsigned char *data, int xsize, int ysize,
    double originx, double originy, double spacingx, double spacingy);

  /**
   * Perform window/level and color mapping operations to produce
   * unsigned char data that can be used as a texture.  See the
   * source file for more information.
   */
  unsigned char *MakeTextureData(
    vtkImageProperty *property, vtkImageData *input, int extent[6],
    int &xsize, int &ysize, int &bytesPerPixel, bool &reuseTexture,
    bool &reuseData);

  /**
   * Compute the coordinates and texture coordinates for the image, given
   * an extent that describes a single slice.
   */
  void MakeTextureGeometry(
    const int extent[6], double coords[12], double tcoords[8]);

  /**
   * Given an extent that describes a slice (it must have unit thickness
   * in one of the three directions), return the dimension indices that
   * correspond to the texture "x" and "y", provide the x, y image size,
   * and provide the texture size (padded to a power of two if the hardware
   * requires).
   */
  virtual void ComputeTextureSize(
    const int extent[6], int &xdim, int &ydim,
    int imageSize[2], int textureSize[2]);

  /**
   * Get the renderer associated with this mapper, or zero if none.
   * This will raise an error if multiple renderers are found.
   */
  vtkRenderer *GetCurrentRenderer();

  /**
   * Get the vtkImage prop associated with this mapper, or zero if none.
   */
  vtkImageSlice *GetCurrentProp() { return this->CurrentProp; }

  /**
   * Get the data-to-world matrix for this mapper, according to the
   * assembly path for its prop.
   */
  vtkMatrix4x4 *GetDataToWorldMatrix();

  /**
   * Get the background color, by using the first color in the
   * supplied lookup table, or black if there is no lookup table.
   */
  void GetBackgroundColor(vtkImageProperty *property, double color[4]);

  vtkTypeBool Border;
  vtkTypeBool Background;
  vtkScalarsToColors *DefaultLookupTable;
  vtkMultiThreader *Threader;
  int NumberOfThreads;
  vtkTypeBool Streaming;

  // The slice.
  vtkPlane *SlicePlane;
  vtkTypeBool SliceAtFocalPoint;
  vtkTypeBool SliceFacesCamera;

  // Information about the image, updated by UpdateInformation
  double DataSpacing[3];
  double DataOrigin[3];
  int DataWholeExtent[6];

  // Set by vtkImageStack when doing multi-pass rendering
  bool MatteEnable;
  bool ColorEnable;
  bool DepthEnable;

private:
  // The prop this mapper is attached to, or zero if none.
  vtkImageSlice *CurrentProp;
  vtkRenderer *CurrentRenderer;

  // The cached data-to-world matrix
  vtkMatrix4x4 *DataToWorldMatrix;

  vtkImageMapper3D(const vtkImageMapper3D&) = delete;
  void operator=(const vtkImageMapper3D&) = delete;

  friend class vtkImageToImageMapper3DFriendship;
};

#endif
