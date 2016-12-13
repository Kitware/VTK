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
 * @class vtkMultiBlockVolumeMapper
 * @brief Mapper to render volumes defined as vtkMultiBlockDataSet.
 *
 * Defers actual rendering to an internal volume mapper (RenderingMapper).
 * All of the blocks of the vtkMultiBlockDataSet are expected to be
 * vtkImageData.
 *
 */
#ifndef vtkMultiBlockVolumeMapper_h
#define vtkMultiBlockVolumeMapper_h

#include <vector>                            // For DataBlocks

#include "vtkTimeStamp.h"                    // For BlockLoadingTime
#include "vtkRenderingVolumeOpenGL2Module.h" // For export macro
#include "vtkAbstractVolumeMapper.h"


class vtkDataObjectTree;
class vtkDataSet;
class vtkImageData;
class vtkMultiBlockDataSet;
class vtkSmartVolumeMapper;

class VTKRENDERINGVOLUMEOPENGL2_EXPORT vtkMultiBlockVolumeMapper :
  public vtkAbstractVolumeMapper
{
public:
  static vtkMultiBlockVolumeMapper *New();
  vtkTypeMacro(vtkMultiBlockVolumeMapper,vtkAbstractVolumeMapper);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   *  \brief API Superclass
   *  \sa vtkAbstractVolumeMapper
   */
  double* GetBounds() VTK_OVERRIDE;
  void SelectScalarArray(int arrayNum) VTK_OVERRIDE;
  void SelectScalarArray(char const* arrayName) VTK_OVERRIDE;
  void SetScalarMode(int ScalarMode);

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

protected:
  vtkMultiBlockVolumeMapper();
  ~vtkMultiBlockVolumeMapper();

  /**
   * Specify the type of data this mapper can handle. This mapper requires
   * vtkDataObjectTree, internally checks whether all the blocks of the data
   * set are vtkImageData.
   *
   * @sa vtkAlgorithm::FillInputPortInformation
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
