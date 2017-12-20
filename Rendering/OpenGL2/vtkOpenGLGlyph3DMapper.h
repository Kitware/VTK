/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLGlyph3DMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLGlyph3DMapper
 * @brief   vtkOpenGLGlyph3D on the GPU.
 *
 * Do the same job than vtkGlyph3D but on the GPU. For this reason, it is
 * a mapper not a vtkPolyDataAlgorithm. Also, some methods of vtkOpenGLGlyph3D
 * don't make sense in vtkOpenGLGlyph3DMapper: GeneratePointIds, old-style
 * SetSource, PointIdsName, IsPointVisible.
 *
 * @sa
 * vtkOpenGLGlyph3D
*/

#ifndef vtkOpenGLGlyph3DMapper_h
#define vtkOpenGLGlyph3DMapper_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkGlyph3DMapper.h"
#include "vtkNew.h" // For vtkNew

class vtkOpenGLGlyph3DHelper;
class vtkBitArray;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLGlyph3DMapper
    : public vtkGlyph3DMapper
{
public:
  static vtkOpenGLGlyph3DMapper* New();
  vtkTypeMacro(vtkOpenGLGlyph3DMapper, vtkGlyph3DMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Method initiates the mapping process. Generally sent by the actor
   * as each frame is rendered.
   */
  void Render(vtkRenderer *ren, vtkActor *a) override;

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *window) override;

  //@{
  /**
   * Get the maximum number of LOD. OpenGL context must be bound.
   * The maximum number of LOD depends on GPU capabilities.
   */
  virtual vtkIdType GetMaxNumberOfLOD() override;

  /**
   * Set the number of LOD.
   */
  virtual void SetNumberOfLOD(vtkIdType nb) override;

  /**
   * Configure LODs. Culling must be enabled.
   * distance have to be a positive value, it is the distance to the camera scaled by
   * the instanced geometry bounding box.
   * targetReduction have to be between 0 and 1, 0 disable decimation, 1 draw a point.
   *
   * @sa vtkDecimatePro::SetTargetReduction
   */
  virtual void SetLODDistanceAndTargetReduction(vtkIdType index, float distance, float targetReduction) override;
  //@}

protected:
  vtkOpenGLGlyph3DMapper();
  ~vtkOpenGLGlyph3DMapper() override;

  /**
   * Render setup
   */
  virtual void Render(vtkRenderer*, vtkActor*, vtkDataSet*);

  /**
   * Send mapper ivars to sub-mapper.
   * \pre mapper_exists: mapper != 0
   */
  void CopyInformationToSubMapper(vtkOpenGLGlyph3DHelper*);

  void SetupColorMapper();

  vtkMapper *ColorMapper;

  class vtkOpenGLGlyph3DMapperEntry;
  class vtkOpenGLGlyph3DMapperSubArray;
  class vtkOpenGLGlyph3DMapperArray;
  vtkOpenGLGlyph3DMapperArray *GlyphValues; // array of value for datasets

  /**
   * Build data structures associated with
   */
  virtual void RebuildStructures(vtkOpenGLGlyph3DMapperSubArray *entry,
    vtkIdType numPts, vtkActor* actor, vtkDataSet* dataset,
    vtkBitArray *maskArray);

  vtkMTimeType BlockMTime; // Last time BlockAttributes was modified.

private:
  vtkOpenGLGlyph3DMapper(const vtkOpenGLGlyph3DMapper&) = delete;
  void operator=(const vtkOpenGLGlyph3DMapper&) = delete;
};

#endif
