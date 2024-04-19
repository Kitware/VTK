// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * \class vtkMultiBlockUnstructuredGridVolumeMapper
 * \brief Mapper to render volumes defined as vtkMultiBlockDataSet.
 *
 */
#ifndef vtkMultiBlockUnstructuredGridVolumeMapper_h
#define vtkMultiBlockUnstructuredGridVolumeMapper_h

#include <vector> // For DataBlocks

#include "vtkNew.h"                          // for ivars
#include "vtkRenderingVolumeOpenGL2Module.h" // For export macro
#include "vtkUnstructuredGridVolumeMapper.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObjectTree;
class vtkDataSet;
class vtkMatrix4x4;
class vtkMultiBlockDataSet;
class vtkUnstructuredGrid;
class vtkRenderWindow;
class vtkProjectedTetrahedraMapper;

class VTKRENDERINGVOLUMEOPENGL2_EXPORT vtkMultiBlockUnstructuredGridVolumeMapper
  : public vtkUnstructuredGridVolumeMapper
{
public:
  static vtkMultiBlockUnstructuredGridVolumeMapper* New();
  vtkTypeMacro(vtkMultiBlockUnstructuredGridVolumeMapper, vtkUnstructuredGridVolumeMapper);
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
   * Set/get whether to use floating-point rendering buffers rather
   * than the default.
   * \sa vtkOpenGLProjectedTetrahedraMapper::setusefloatingpointframebuffer
   */
  void SetUseFloatingPointFrameBuffer(bool use);
  vtkGetMacro(UseFloatingPointFrameBuffer, bool);

  ///@{
  /**
   * blending mode api from vtkUnstructuredGridVolumemapper
   * \sa vtkvolumemapper::setblendmode
   */
  void SetBlendMode(int mode) override;
  ///@}

protected:
  vtkMultiBlockUnstructuredGridVolumeMapper();
  ~vtkMultiBlockUnstructuredGridVolumeMapper() override;

  /**
   * Specify the type of data this mapper can handle. This mapper requires
   * vtkDataObjectTree, internally checks whether all the blocks of the data
   * set are vtkImageData.
   *
   * \sa vtkAlgorithm::FillInputPortInformation
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  bool UseFloatingPointFrameBuffer;

  vtkRenderWindow* DebugWin;
  vtkRenderer* DebugRen;
  vtkNew<vtkMatrix4x4> TempMatrix4x4;

private:
  /**
   * Traverse the vtkMultiBlockDataSet and create shallow copies to its valid blocks
   * (vtkUnstructuredGrid blocks). References are kept in a vector which is sorted back-to-front
   * on every render call.
   */
  void LoadDataSet();

  /**
   * Creates a mapper per data block and tries to load the data. If allocating
   * fails in any of the mappers, an additional mapper instance is created
   * (FallBackMapper) and used for rendering (single mapper). The FallBackMapper
   * instance is created and used in single-mapper-mode for convenience, just to
   * keep using the Mappers vector for sorting without having to manage their
   * data.
   */
  void CreateMappers(vtkDataObjectTree* input);

  vtkDataObjectTree* GetDataObjectTreeInput();

  /**
   * Compute the bounds enclosing all of the blocks in the dataset.
   */
  void ComputeBounds();

  /**
   * Sort loaded vtkUnstructuredGrid blocks back-to-front.
   */
  void SortMappers(vtkRenderer* ren, vtkMatrix4x4* volumeMat);

  void ClearMappers();

  /**
   * Create and setup a proxy rendering-mapper with the current flags.
   */
  vtkProjectedTetrahedraMapper* CreateMapper();

  vtkMultiBlockUnstructuredGridVolumeMapper(
    const vtkMultiBlockUnstructuredGridVolumeMapper&) = delete;
  void operator=(const vtkMultiBlockUnstructuredGridVolumeMapper&) = delete;

  /////////////////////////////////////////////////////////////////////////////

  typedef std::vector<vtkProjectedTetrahedraMapper*> MapperVec;
  MapperVec Mappers;

  vtkMTimeType BlockLoadingTime;
  vtkMTimeType BoundsComputeTime;
};
VTK_ABI_NAMESPACE_END
#endif
