/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageStack.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageStack
 * @brief   manages a stack of composited images
 *
 * vtkImageStack manages the compositing of a set of images. Each image
 * is assigned a layer number through its property object, and it is
 * this layer number that determines the compositing order: images with
 * a higher layer number are drawn over top of images with a lower layer
 * number.  The image stack has a SetActiveLayer method for controlling
 * which layer to use for interaction and picking.
 * @par Thanks:
 * Thanks to David Gobbi at the Seaman Family MR Centre and Dept. of Clinical
 * Neurosciences, Foothills Medical Centre, Calgary, for providing this class.
 * @sa
 * vtkImageMapper3D vtkImageProperty vtkProp3D
*/

#ifndef vtkImageStack_h
#define vtkImageStack_h

#include "vtkRenderingImageModule.h" // For export macro
#include "vtkImageSlice.h"

class vtkImageSliceCollection;
class vtkImageProperty;
class vtkImageMapper3D;
class vtkCollection;

class VTKRENDERINGIMAGE_EXPORT vtkImageStack : public vtkImageSlice
{
public:
  vtkTypeMacro(vtkImageStack,vtkImageSlice);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkImageStack *New();

  /**
   * Add an image to the stack.  If the image is already present, then
   * this method will do nothing.
   */
  void AddImage(vtkImageSlice *prop);

  /**
   * Remove an image from the stack.  If the image is not present, then
   * this method will do nothing.
   */
  void RemoveImage(vtkImageSlice *prop);

  /**
   * Check if an image is present.  The returned value is one or zero.
   */
  int HasImage(vtkImageSlice *prop);

  /**
   * Get the list of images as a vtkImageSliceCollection.
   */
  vtkImageSliceCollection *GetImages() { return this->Images; }

  //@{
  /**
   * Set the active layer number.  This is the layer that will be
   * used for picking and interaction.
   */
  vtkSetMacro(ActiveLayer, int);
  int GetActiveLayer() { return this->ActiveLayer; }
  //@}

  /**
   * Get the active image.  This will be the topmost image whose
   * LayerNumber is the ActiveLayer.  If no image matches, then NULL
   * will be returned.
   */
  vtkImageSlice *GetActiveImage();

  /**
   * Get the mapper for the currently active image.
   */
  vtkImageMapper3D *GetMapper() override;

  /**
   * Get the property for the currently active image.
   */
  vtkImageProperty *GetProperty() override;

  //@{
  /**
   * Get the combined bounds of all of the images.
   */
  double *GetBounds() override;
  void GetBounds(double bounds[6]) { this->vtkProp3D::GetBounds( bounds ); };
  //@}

  /**
   * Return the max MTime of all the images.
   */
  vtkMTimeType GetMTime() override;

  /**
   * Return the mtime of anything that would cause the rendered image to
   * appear differently. Usually this involves checking the mtime of the
   * prop plus anything else it depends on such as properties, mappers,
   * etc.
   */
  vtkMTimeType GetRedrawMTime() override;

  /**
   * Shallow copy of this prop. Overloads the virtual vtkProp method.
   */
  void ShallowCopy(vtkProp *prop) override;

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
  int RenderOverlay(vtkViewport *viewport) override;
  int RenderOpaqueGeometry(vtkViewport *viewport) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport *viewport) override;
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  vtkTypeBool HasTranslucentPolygonalGeometry() override;

  /**
   * Release any resources held by this prop.
   */
  void ReleaseGraphicsResources(vtkWindow *win) override;

  //@{
  /**
   * Methods for traversing the stack as if it was an assembly.
   * The traversal only gives the view prop for the active layer.
   */
  void InitPathTraversal() override;
  vtkAssemblyPath *GetNextPath() override;
  int GetNumberOfPaths() override;
  //@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Used to construct assembly paths and perform part traversal.
   */
  void BuildPaths(vtkAssemblyPaths *paths, vtkAssemblyPath *path) override;

protected:
  vtkImageStack();
  ~vtkImageStack() override;

  void SetMapper(vtkImageMapper3D *mapper);
  void SetProperty(vtkImageProperty *property);

  void PokeMatrices(vtkMatrix4x4 *matrix);
  void UpdatePaths();

  vtkTimeStamp PathTime;
  vtkCollection *ImageMatrices;
  vtkImageSliceCollection *Images;
  int ActiveLayer;

private:
  vtkImageStack(const vtkImageStack&) = delete;
  void operator=(const vtkImageStack&) = delete;
};

#endif
