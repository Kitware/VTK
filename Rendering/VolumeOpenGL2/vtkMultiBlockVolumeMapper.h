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
 * clipping planes appropriately. At render time, blocks are sorted back-to-front
 * and each block is rendered independently. The mapper defers actual rendering to its
 * internal vtkSmartVolumeMapper.
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
  int GetVectorMode();
  void SetVectorComponent(int component);
  int GetVectorComponent();
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
  int GetBlendMode() VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Cropping API from vtkVolumeMapper
   * \sa vtkVolumeMapper::SetCropping
   */
  void SetCropping(int mode) VTK_OVERRIDE;
  int GetCropping() VTK_OVERRIDE;

  /**
   * \sa vtkVolumeMapper::SetCroppingRegionPlanes
   */
  void SetCroppingRegionPlanes(double arg1, double arg2, double arg3,
    double arg4, double arg5, double arg6) VTK_OVERRIDE;
  void SetCroppingRegionPlanes(double *planes) VTK_OVERRIDE;
  void GetCroppingRegionPlanes(double *planes) VTK_OVERRIDE;
  double *GetCroppingRegionPlanes() VTK_OVERRIDE;

  /**
   * \sa vtkVolumeMapper::SetCroppingRegionFlags
   */
  void SetCroppingRegionFlags(int mode) VTK_OVERRIDE;
  int GetCroppingRegionFlags() VTK_OVERRIDE;
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
  void LoadBlocks();

  vtkDataObjectTree* GetDataObjectTreeInput();

  /**
   * Compute the bounds enclosing all of the blocks in the dataset.
   */
  void ComputeBounds();

  /**
   * Sort loaded vtkImageData blocks back-to-front.
   */
  void SortBlocks(vtkRenderer* ren, vtkMatrix4x4* volumeMat);

  void ClearBlocks();

  vtkMultiBlockVolumeMapper(const vtkMultiBlockVolumeMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMultiBlockVolumeMapper&) VTK_DELETE_FUNCTION;

  /////////////////////////////////////////////////////////////////////////////

  vtkSmartVolumeMapper* RenderingMapper;

  typedef std::vector<vtkImageData*> BlockVec;
  BlockVec DataBlocks;

  vtkTimeStamp BlockLoadingTime;
  vtkTimeStamp BoundsComputeTime;
};
#endif
