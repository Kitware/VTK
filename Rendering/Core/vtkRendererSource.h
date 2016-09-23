/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRendererSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkRendererSource
 * @brief   take a renderer's image and/or depth map into the pipeline
 *
 *
 * vtkRendererSource is a source object whose input is a renderer's image
 * and/or depth map, which is then used to produce an output image. This
 * output can then be used in the visualization pipeline. You must explicitly
 * send a Modify() to this object to get it to reload its data from the
 * renderer. Consider also using vtkWindowToImageFilter instead of this
 * class.
 *
 * By default, the data placed into the output is the renderer's image RGB
 * values (these color scalars are represented by unsigned chars, one per
 * color channel). Optionally, you can also grab the image depth (e.g.,
 * z-buffer) values, and include it in the output in one of three ways. 1)
 * First, when the data member DepthValues is enabled, a separate float array
 * of these depth values is included in the output point data with array name
 * "ZBuffer". 2) If DepthValuesInScalars is enabled, then the z-buffer values
 * are shifted and scaled to fit into an unsigned char and included in the
 * output image (so the output image pixels are four components RGBZ). Note
 * that DepthValues and and DepthValuesInScalars can be enabled
 * simultaneously if desired. Finally 3) if DepthValuesOnly is enabled, then
 * the output image consists only of the z-buffer values represented by a
 * single component float array; and the data members DepthValues and
 * DepthValuesInScalars are ignored.
 *
 * @sa
 * vtkWindowToImageFilter vtkRendererPointCloudSource vtkRenderer
 * vtkImageData vtkDepthImageToPointCloud
*/

#ifndef vtkRendererSource_h
#define vtkRendererSource_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkAlgorithm.h"
#include "vtkImageData.h" // makes things a bit easier

class vtkRenderer;

class VTKRENDERINGCORE_EXPORT vtkRendererSource : public vtkAlgorithm
{
public:
  static vtkRendererSource *New();
  vtkTypeMacro(vtkRendererSource, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Return the MTime also considering the Renderer.
   */
  vtkMTimeType GetMTime();

  /**
   * Indicates what renderer to get the pixel data from.
   */
  void SetInput(vtkRenderer*);

  //@{
  /**
   * Returns which renderer is being used as the source for the pixel data.
   */
  vtkGetObjectMacro(Input, vtkRenderer);
  //@}

  //@{
  /**
   * Use the entire RenderWindow as a data source or just the Renderer.
   * The default is zero, just the Renderer.
   */
  vtkSetMacro(WholeWindow, int);
  vtkGetMacro(WholeWindow, int);
  vtkBooleanMacro(WholeWindow, int);
  //@}

  //@{
  /**
   * If this flag is on, then filter execution causes a render first.
   */
  vtkSetMacro(RenderFlag, int);
  vtkGetMacro(RenderFlag, int);
  vtkBooleanMacro(RenderFlag, int);
  //@}

  //@{
  /**
   * A boolean value to control whether to grab z-buffer
   * (i.e., depth values) along with the image data. The z-buffer data
   * is placed into a field data attributes named "ZBuffer" .
   */
  vtkSetMacro(DepthValues, int);
  vtkGetMacro(DepthValues, int);
  vtkBooleanMacro(DepthValues, int);
  //@}

  //@{
  /**
   * A boolean value to control whether to grab z-buffer
   * (i.e., depth values) along with the image data. The z-buffer data
   * is placed in the scalars as a fourth Z component (shift and scaled
   * to map the full 0..255 range).
   */
  vtkSetMacro(DepthValuesInScalars, int);
  vtkGetMacro(DepthValuesInScalars, int);
  vtkBooleanMacro(DepthValuesInScalars, int);
  //@}

  //@{
  /**
   * A boolean value to control whether to grab only the z-buffer (i.e.,
   * depth values) without the associated image (color scalars) data. If
   * enabled, the output data contains only a depth image which is the
   * z-buffer values represented by float values. By default, this is
   * disabled. Note that if enabled, then the DepthValues and
   * DepthValuesInScalars are ignored.
   */
  vtkSetMacro(DepthValuesOnly, int);
  vtkGetMacro(DepthValuesOnly, int);
  vtkBooleanMacro(DepthValuesOnly, int);
  //@}

  /**
   * Get the output data object for a port on this algorithm.
   */
  vtkImageData* GetOutput();

  /**
   * see vtkAlgorithm for details
   */
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

protected:
  vtkRendererSource();
  ~vtkRendererSource();

  void RequestData(vtkInformation* request,
                   vtkInformationVector** inputVector,
                   vtkInformationVector* outputVector);
  virtual void RequestInformation (vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

  vtkRenderer *Input;
  int WholeWindow;
  int RenderFlag;
  int DepthValues;
  int DepthValuesInScalars;
  int DepthValuesOnly;

  // see algorithm for more info
  virtual int FillOutputPortInformation(int port, vtkInformation* info);

private:
  vtkRendererSource(const vtkRendererSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRendererSource&) VTK_DELETE_FUNCTION;
};

#endif
