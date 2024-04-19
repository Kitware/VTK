// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
 * discontinuous resolution between blocks.  Jittering is enabled by default.
 * Jittering is only supported in GPURenderMode.
 *
 */
#ifndef vtkMultiBlockVolumeMapper_h
#define vtkMultiBlockVolumeMapper_h

#include <vector> // For DataBlocks

#include "vtkNew.h"                          // for ivar
#include "vtkRenderingVolumeOpenGL2Module.h" // For export macro
#include "vtkVolumeMapper.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObjectTree;
class vtkDataSet;
class vtkImageData;
class vtkMatrix4x4;
class vtkMultiBlockDataSet;
class vtkRenderWindow;
class vtkSmartVolumeMapper;

class VTKRENDERINGVOLUMEOPENGL2_EXPORT vtkMultiBlockVolumeMapper : public vtkVolumeMapper
{
public:
  static vtkMultiBlockVolumeMapper* New();
  vtkTypeMacro(vtkMultiBlockVolumeMapper, vtkVolumeMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   *  \brief API Superclass
   *  \sa vtkAbstractVolumeMapper
   */
  double* GetBounds() override;
  using vtkAbstractVolumeMapper::GetBounds;

  void SelectScalarArray(int arrayNum) override;
  void SelectScalarArray(char const* arrayName) override;
  void SetScalarMode(int ScalarMode) override;
  void SetArrayAccessMode(int accessMode) override;

  /**
   * Render the current dataset.
   *
   * \warning Internal method - not intended for general use, do
   * NOT use this method outside of the rendering process.
   */
  void Render(vtkRenderer* ren, vtkVolume* vol) override;

  /**
   * \warning Internal method - not intended for general use, do
   * NOT use this method outside of the rendering process.
   */
  void ReleaseGraphicsResources(vtkWindow* window) override;
  ///@}

  ///@{
  /**
   * VectorMode interface exposed from vtkSmartVolumeMapper.
   */
  void SetVectorMode(int mode);
  vtkGetMacro(VectorMode, int);
  void SetVectorComponent(int component);
  vtkGetMacro(VectorComponent, int);
  ///@}

  ///@{
  /**
   * Blending mode API from vtkVolumeMapper
   * \sa vtkVolumeMapper::SetBlendMode
   */
  void SetBlendMode(int mode) override;
  ///@}

  ///@{
  /**
   * ComputeNormalFromOpacity exposed
   * \sa vtkVolumeMapper::SetComputeNormalFromOpacity
   */
  void SetComputeNormalFromOpacity(bool val) override;
  ///@}

  ///@{
  /**
   * @copydoc vtkSmartVolumeMapper::SetGlobalIlluminationReach(float)
   */
  void SetGlobalIlluminationReach(float val);
  vtkGetMacro(GlobalIlluminationReach, float);
  ///@}

  ///@{
  /**
   * @copydoc vtkSmartVolumeMapper::SetVolumetricScatteringBlending(float)
   */
  void SetVolumetricScatteringBlending(float val);
  vtkGetMacro(VolumetricScatteringBlending, float);
  ///@}

  ///@{
  /**
   * Cropping API from vtkVolumeMapper
   * \sa vtkVolumeMapper::SetCropping
   */
  void SetCropping(vtkTypeBool mode) override;

  /**
   * \sa vtkVolumeMapper::SetCroppingRegionPlanes
   */
  void SetCroppingRegionPlanes(
    double arg1, double arg2, double arg3, double arg4, double arg5, double arg6) override;
  void SetCroppingRegionPlanes(const double* planes) override;

  /**
   * \sa vtkVolumeMapper::SetCroppingRegionFlags
   */
  void SetCroppingRegionFlags(int mode) override;
  ///@}

  ///@{
  /**
   * Forwarded to internal vtkSmartVolumeMappers used.
   * @sa vtkSmartVolumeMapper::SetRequestedRenderMode.
   */
  void SetRequestedRenderMode(int);
  ///@}

  ///@{
  /**
   * \sa vtkSmartVolumeMapper::SetTransfer2DYAxisArray
   */
  void SetTransfer2DYAxisArray(const char* a);
  ///@}

protected:
  vtkMultiBlockVolumeMapper();
  ~vtkMultiBlockVolumeMapper() override;

  /**
   * Specify the type of data this mapper can handle. This mapper requires
   * vtkDataObjectTree, internally checks whether all the blocks of the data
   * set are vtkImageData.
   *
   * \sa vtkAlgorithm::FillInputPortInformation
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  vtkRenderWindow* DebugWin;
  vtkRenderer* DebugRen;

  vtkNew<vtkMatrix4x4> TempMatrix4x4;

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

  vtkMultiBlockVolumeMapper(const vtkMultiBlockVolumeMapper&) = delete;
  void operator=(const vtkMultiBlockVolumeMapper&) = delete;

  /////////////////////////////////////////////////////////////////////////////

  typedef std::vector<vtkSmartVolumeMapper*> MapperVec;
  MapperVec Mappers;
  vtkSmartVolumeMapper* FallBackMapper;

  vtkMTimeType BlockLoadingTime;
  vtkMTimeType BoundsComputeTime;

  int VectorMode;
  int VectorComponent;
  int RequestedRenderMode;

  /**
   * Secondary rays ambient/global adjustment coefficient
   */
  float GlobalIlluminationReach = 0.0;

  /**
   * Blending coefficient between surfacic and volumetric models in GPU Mapper
   */
  float VolumetricScatteringBlending = 0.0;

  char* Transfer2DYAxisArray;
};
VTK_ABI_NAMESPACE_END
#endif
