/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLPointGaussianMapper
 * @brief   draw PointGaussians using imposters
 *
 * An OpenGL mapper that uses imposters to draw PointGaussians. Supports
 * transparency and picking as well.
*/

#ifndef vtkOpenGLPointGaussianMapper_h
#define vtkOpenGLPointGaussianMapper_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkPointGaussianMapper.h"
#include <vector> // for ivar

class vtkOpenGLPointGaussianMapperHelper;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLPointGaussianMapper : public vtkPointGaussianMapper
{
public:
  static vtkOpenGLPointGaussianMapper* New();
  vtkTypeMacro(vtkOpenGLPointGaussianMapper, vtkPointGaussianMapper)
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *) override;

  /**
   * Is this mapper opqaue? currently always false.
   */
  bool GetIsOpaque() override;

  /**
   * This calls RenderPiece (in a for loop if streaming is necessary).
   */
  void Render(vtkRenderer *ren, vtkActor *act) override;

  /**
   * allows a mapper to update a selections color buffers
   * Called from a prop which in turn is called from the selector
   */
  void ProcessSelectorPixelBuffers(vtkHardwareSelector *sel,
    std::vector<unsigned int> &pixeloffsets,
    vtkProp *prop) override;

protected:
  vtkOpenGLPointGaussianMapper();
  ~vtkOpenGLPointGaussianMapper() override;

  void ReportReferences(vtkGarbageCollector* collector) override;

  std::vector<vtkOpenGLPointGaussianMapperHelper *> Helpers;
  vtkOpenGLPointGaussianMapperHelper *CreateHelper();
  void CopyMapperValuesToHelper(
    vtkOpenGLPointGaussianMapperHelper *helper);

  vtkTimeStamp HelperUpdateTime;
  vtkTimeStamp ScaleTableUpdateTime;
  vtkTimeStamp OpacityTableUpdateTime;

  // unused
  void RenderPiece(vtkRenderer *, vtkActor *) override {};

  void RenderInternal(vtkRenderer *, vtkActor *);

  // create the table for opacity values
  void BuildOpacityTable();

  // create the table for scale values
  void BuildScaleTable();

  float *OpacityTable; // the table
  double OpacityScale; // used for quick lookups
  double OpacityOffset; // used for quick lookups
  float *ScaleTable; // the table
  double ScaleScale; // used for quick lookups
  double ScaleOffset; // used for quick lookups

  /**
   * We need to override this method because the standard streaming
   * demand driven pipeline may not be what we need as we can handle
   * hierarchical data as input
   */
  vtkExecutive* CreateDefaultExecutive() override;

  /**
   * Need to define the type of data handled by this mapper.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Need to loop over the hierarchy to compute bounds
   */
  void ComputeBounds() override;

  // used by the hardware selector
  std::vector<std::vector<unsigned int>> PickPixels;

private:
  vtkOpenGLPointGaussianMapper(const vtkOpenGLPointGaussianMapper&) = delete;
  void operator=(const vtkOpenGLPointGaussianMapper&) = delete;
};

#endif
