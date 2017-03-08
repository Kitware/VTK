/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockVolumeMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * \class vtkMultiBlockVolumeMapper
 * \brief Mapper to render volumes defined as vtkMultiBlockDataSet.
 *
 * vtkMultiBlockVolumeMapper renders vtkMultiBlockDataSet instances containing
 * vtkImageData blocks (all of the blocks are expected to be vtkImageData). Bounds
 * containing the full set of blocks are computed so that vtkRenderer can adjust the
 * clipping planes appropriately.
 *
 * This mapper creates an instance of vtkSmartVolumeMapper per block to which
 * it defers the actual rendering.  At render time, blocks (mappers) are sorted
 * back-to-front and each block is rendered independently.  It attempts to load all
 * of the blocks at the same time but tries to catch allocation errors in which case
 * it falls back to using a single mapper instance and reloading data for each block.
 *
 * Jittering is used to alleviate seam artifacts at the block edges due to the
 * discontinuous resolution between blocks.  Jittering is disabled by default until
 * valid resolution is set (e.g. x > 0 && y > 0).  Jittering is only supported in
 * GPURenderMode.
 *
 */
#ifndef vtkMultiBlockVolumeMapper_h
#define vtkMultiBlockVolumeMapper_h

#include <vector>                            // For DataBlocks

#include "vtkTimeStamp.h"                    // For BlockLoadingTime
#include "vtkRenderingVolumeOpenGL2Module.h" // For export macro
#include "vtkVolumeMapper.h"


class vtkDataObjectTree;
class vtkDataSet;
class vtkImageData;
class vtkMultiBlockDataSet;
class vtkSmartVolumeMapper;

class VTKRENDERINGVOLUMEOPENGL2_EXPORT vtkMultiBlockVolumeMapper :
  public vtkVolumeMapper
{
public:
  static vtkMultiBlockVolumeMapper *New();
  vtkTypeMacro(vtkMultiBlockVolumeMapper,vtkVolumeMapper);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   *  \brief API Superclass
   *  \sa vtkAbstractVolumeMapper
   */
  double* GetBounds() VTK_OVERRIDE;
  using vtkAbstractVolumeMapper::GetBounds;

  void SelectScalarArray(int arrayNum) VTK_OVERRIDE;
  void SelectScalarArray(char const* arrayName) VTK_OVERRIDE;
  void SetScalarMode(int ScalarMode) VTK_OVERRIDE;
  void SetArrayAccessMode(int accessMode) VTK_OVERRIDE;

  /**
   * Render the current dataset.
   *
   * \warning Internal method - not intended for general use, do
   * NOT use this method outside of the rendering process.
   */
  void Render(vtkRenderer* ren, vtkVolume* vol) VTK_OVERRIDE;

  /**
   * \warning Internal method - not intended for general use, do
   * NOT use this method outside of the rendering process.
   */
  void ReleaseGraphicsResources(vtkWindow* window) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * VectorMode interface exposed from vtkSmartVolumeMapper.
   */
  void SetVectorMode(int mode);
  vtkGetMacro(VectorMode, int);
  void SetVectorComponent(int component);
  vtkGetMacro(VectorComponent, int);
  //@}

  /**
   * Set the resolution of the noise texture used for ray jittering (viewport's
   * resolution is normally a good choice).  In this mapper jittering is used to
   * alleviate seam artifacts at the block edges due to discontinuous resolution
   * between blocks.  Jittering is disabled by default until valid resolution is
   * set (e.g. x > 0 && y > 0).
   */
  void SetJitteringResolution(int x, int y);

  //@{
  /**
   * Blending mode API from vtkVolumeMapper
   * \sa vtkVolumeMapper::SetBlendMode
   */
  void SetBlendMode(int mode) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Cropping API from vtkVolumeMapper
   * \sa vtkVolumeMapper::SetCropping
   */
  void SetCropping(int mode) VTK_OVERRIDE;

  /**
   * \sa vtkVolumeMapper::SetCroppingRegionPlanes
   */
  void SetCroppingRegionPlanes(double arg1, double arg2, double arg3,
    double arg4, double arg5, double arg6) VTK_OVERRIDE;
  void SetCroppingRegionPlanes(double *planes) VTK_OVERRIDE;

  /**
   * \sa vtkVolumeMapper::SetCroppingRegionFlags
   */
  void SetCroppingRegionFlags(int mode) VTK_OVERRIDE;
  //@}

protected:
  vtkMultiBlockVolumeMapper();
  ~vtkMultiBlockVolumeMapper();

  /**
   * Specify the type of data this mapper can handle. This mapper requires
   * vtkDataObjectTree, internally checks whether all the blocks of the data
   * set are vtkImageData.
   *
   * \sa vtkAlgorithm::FillInputPortInformation
   */
  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

private:
  /**
   * Traverse the vtkMultiBlockDataSet and create shallow copies to its valid blocks
   * (vtkImageData blocks). References are kept in a vector which is sorted back-to-front
   * on every render call.
   */
  void LoadDataSet(vtkRenderer* ren, vtkVolume* vol);

  /**
   * Creates a mapper per data block and tries to load the data. If allocating
   * fails in any of the mappers, an additional mapper instance is created
   * (FallBackMapper) and used for rendering (single mapper). The FallBackMapper
   * instance is created and used in single-mapper-mode for convenience, just to
   * keep using the Mappers vector for sorting without having to manage their
   * data.
   */
  void CreateMappers(vtkDataObjectTree* input, vtkRenderer* ren, vtkVolume* vol);

  void ApplyJitteringResolution(vtkSmartVolumeMapper* mapper);

  vtkDataObjectTree* GetDataObjectTreeInput();

  /**
   * Compute the bounds enclosing all of the blocks in the dataset.
   */
  void ComputeBounds();

  /**
   * Sort loaded vtkImageData blocks back-to-front.
   */
  void SortMappers(vtkRenderer* ren, vtkMatrix4x4* volumeMat);

  void ClearMappers();

  /**
   * Create and setup a proxy rendering-mapper with the current flags.
   */
  vtkSmartVolumeMapper* CreateMapper();

  vtkMultiBlockVolumeMapper(const vtkMultiBlockVolumeMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMultiBlockVolumeMapper&) VTK_DELETE_FUNCTION;

  /////////////////////////////////////////////////////////////////////////////

  typedef std::vector<vtkSmartVolumeMapper*> MapperVec;
  MapperVec Mappers;
  vtkSmartVolumeMapper* FallBackMapper;

  vtkTimeStamp BlockLoadingTime;
  vtkTimeStamp BoundsComputeTime;

  int JitteringSizeX;
  int JitteringSizeY;

  int VectorMode;
  int VectorComponent;
};
#endif
