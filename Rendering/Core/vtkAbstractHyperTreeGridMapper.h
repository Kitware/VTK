// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAbstractHyperTreeGridMapper
 * @brief   Abstract class for a HyperTreeGrid mapper
 *
 * vtkAbstractHyperTreeGridMapper is the abstract definition of a HyperTreeGrid mapper.
 * Several  basic types of volume mappers are supported.
 *
 * @sa
 * vtkHyperTreeGrid vtkUniformHyperTreeGrid
 *
 * @par Thanks:
 * This class was written by Philippe Pebay and Meriadeg Perrinel,
 * NexGen Analytics 2018
 * This worked was based on an idea of Guenole Harel and Jacques-Bernard Lekien
 * This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkAbstractHyperTreeGridMapper_h
#define vtkAbstractHyperTreeGridMapper_h

#include "vtkAbstractVolumeMapper.h"
#include "vtkRenderingCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkBitArray;
class vtkDataArray;
class vtkMatrix4x4;
class vtkScalarsToColors;
class vtkRenderer;
class vtkUniformHyperTreeGrid;

class VTKRENDERINGCORE_EXPORT vtkAbstractHyperTreeGridMapper : public vtkAbstractVolumeMapper
{
public:
  vtkTypeMacro(vtkAbstractHyperTreeGridMapper, vtkAbstractVolumeMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the input data or connection
   */
  virtual void SetInputData(vtkUniformHyperTreeGrid*);
  void SetInputConnection(int, vtkAlgorithmOutput*) override;
  void SetInputConnection(vtkAlgorithmOutput* input) override
  {
    this->SetInputConnection(0, input);
  }
  vtkUniformHyperTreeGrid* GetInput();
  ///@}

  ///@{
  /**
   * Set/Get the renderer attached to this HyperTreeGrid mapper
   */
  void SetRenderer(vtkRenderer*);
  vtkGetObjectMacro(Renderer, vtkRenderer);
  ///@}

  /**
   * Set the scale factor
   */
  vtkSetMacro(Scale, double);

  ///@{
  /**
   * Set/Get the color map attached to this HyperTreeGrid mapper
   * A linear lookup table is provided by default
   */
  void SetColorMap(vtkScalarsToColors*);
  vtkGetObjectMacro(ColorMap, vtkScalarsToColors);
  ///@}

  ///@{
  /**
   * Specify range in terms of scalar minimum and maximum.
   * These values are used to map scalars into lookup table
   * Has no effect when dimension > 2
   * Used only when ColorMap is a lookup table instance
   */
  void SetScalarRange(double, double);
  void SetScalarRange(double*);
  vtkGetVectorMacro(ScalarRange, double, 2);
  ///@}

  /**
   * Get image size
   */
  vtkGetVectorMacro(ViewportSize, int, 2);

  /**
   * Get the mtime of this object.
   */
  vtkMTimeType GetMTime() override;

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow*) override {}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Render the volume
   */
  void Render(vtkRenderer*, vtkVolume*) override = 0;

protected:
  vtkAbstractHyperTreeGridMapper();
  ~vtkAbstractHyperTreeGridMapper() override;

  /**
   * Restrict input type to vtkUniformHyperTreeGrid instances
   */
  int FillInputPortInformation(int, vtkInformation*) override;

  /**
   * Reference to input scalars
   */
  vtkDataArray* Scalars;

  ///@{
  /**
   * Keep track of coordinate conversion matrices
   */
  vtkMatrix4x4* WorldToViewMatrix;
  vtkMatrix4x4* ViewToWorldMatrix;
  ///@}

  /**
   * Keep track of whether pixelize grid is current
   */
  bool MustUpdateGrid;

  /**
   * Orientation of input grid when dimension < 3
   */
  unsigned int Orientation;

  /**
   * Reference to the renderer being used
   */
  vtkRenderer* Renderer;

  /**
   * Scalar range for color lookup table when dimension < 3
   */
  double ScalarRange[2];

  /**
   * Color map used only when dimension < 3
   */
  vtkScalarsToColors* ColorMap;

  /**
   * Scale factor for adaptive view
   */
  double Scale;

  /**
   * Radius parameter for adaptive view
   */
  double Radius;

  /**
   * First axis parameter for adaptive view
   */
  unsigned int Axis1;

  /**
   * Second axis parameter for adaptive view
   */
  unsigned int Axis2;

  /**
   * Maximum depth parameter for adaptive view
   */
  int LevelMax;

  /**
   * Parallel projection parameter for adaptive view
   */
  bool ParallelProjection;

  /**
   * Last camera parallel scale for adaptive view
   */
  double LastCameraParallelScale;

  /**
   * Viewport size for computed image
   */
  int ViewportSize[2];

  /**
   * Last renderer size parameters for adaptive view
   */
  int LastRendererSize[2];

  /**
   * Last camera focal point coordinates for adaptive view
   */
  double LastCameraFocalPoint[3];

  /**
   * Keep track of current view orientation
   */
  int ViewOrientation;

  /**
   * Internal frame buffer
   */
  unsigned char* FrameBuffer;

  /**
   * Internal z-buffer
   */
  float* ZBuffer;

private:
  vtkAbstractHyperTreeGridMapper(const vtkAbstractHyperTreeGridMapper&) = delete;
  void operator=(const vtkAbstractHyperTreeGridMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
