// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class  vtkVolumeTexture
 * @brief  Creates and manages the volume texture rendered by
 * vtkOpenGLGPUVolumeRayCastMapper.
 *
 * Wraps a vtkTextureObject for which it selects the appropriate format
 * (depending on the input vtkDataArray type, number of components, etc.) and
 * loads input data. The class maintains a set of members of interest to the
 * parent mapper, such as:
 *
 * * Active vtkDataArray scalar range.
 * * Volume's scale and bias (pixel transfer functions).
 * * HandleLargeDataType flag.
 * * Texture to data transformations.
 * * Block extents
 * * Block loaded bounds
 *
 * This class supports streaming the volume data in separate blocks to make it
 * fit in graphics memory (sometimes referred to as bricking). The data is split
 * into a user-defined number of blocks in such a way that a single sub-block
 * (brick) fits completely into GPU memory.  A stride is passed to OpenGL so
 * that it can access the underlying vtkDataArray adequately for each of the
 * blocks to be streamed into GPU memory (back-to-front for correct
 * composition).
 *
 * Streaming the volume as separate texture bricks certainly imposes a
 * performance trade-off but acts as a graphics memory expansion scheme for
 * devices that would not be able to render higher resoulution volumes
 * otherwise.
 *
 * @warning There are certain caveats when texture streaming is enabled, given
 * the locality constraint that rendering a single block imposes.
 *
 * - Quality might suffer near the block seams with ShadeOn() (gradient
 * computation at the boundaries needs adjustment).
 *
 * - Not all of the features supported by the mapper currently work correctly.
 *   This is a list of known issues:
 *   - Blending modes such as average and additive might compute a different
 *      value near the edges.
 *
 * - Future work will extend the API to be able to compute an ideal number of
 *   partitions and extents based on the platform capabilities.
 *
 * @warning This is an internal class of vtkOpenGLGPUVolumeRayCastMapper. It
 * assumes there is an active OpenGL context in methods involving GL calls
 * (MakeCurrent() is expected to be called in the mapper beforehand).
 */

#ifndef vtkVolumeTexture_h
#define vtkVolumeTexture_h
#include <map>    // For ImageDataBlockMap
#include <vector> // For ImageDataBlocks

#include "vtkMatrix4x4.h" // For vtkMatrix4
#include "vtkNew.h"       // For vtkNew
#include "vtkObject.h"
#include "vtkRenderingVolumeOpenGL2Module.h" // For export macro
#include "vtkSmartPointer.h"                 // For SmartPointer
#include "vtkTimeStamp.h"                    // For UploadTime
#include "vtkTuple.h"                        // For Size6 and Size3

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkDataSet;
class vtkImageData;
class vtkRenderer;
class vtkTextureObject;
class vtkVolumeProperty;
class vtkWindow;

class VTKRENDERINGVOLUMEOPENGL2_EXPORT vtkVolumeTexture : public vtkObject
{
  typedef vtkTuple<int, 6> Size6;
  typedef vtkTuple<int, 3> Size3;

public:
  static vtkVolumeTexture* New();

  struct VolumeBlock
  {
    VolumeBlock(vtkDataSet* dataset, vtkTextureObject* tex, Size3 const& texSize)
    {
      // Block extent is stored in vtkDataSet
      DataSet = dataset;
      TextureObject = tex;
      TextureSize = texSize;
      TupleIndex = 0;

      this->Extents[0] = VTK_INT_MAX;
      this->Extents[1] = VTK_INT_MIN;
      this->Extents[2] = VTK_INT_MAX;
      this->Extents[3] = VTK_INT_MIN;
      this->Extents[4] = VTK_INT_MAX;
      this->Extents[5] = VTK_INT_MIN;
    }

    vtkDataSet* DataSet;
    vtkTextureObject* TextureObject;
    Size3 TextureSize;
    vtkIdType TupleIndex;
    vtkNew<vtkMatrix4x4> TextureToDataset;
    vtkNew<vtkMatrix4x4> TextureToDatasetInv;

    float CellStep[3];
    double DatasetStepSize[3];

    /**
     * LoadedBounds are corrected for cell-data (if that is the case). So they
     * are not equivalent to vtkDataSet::GetBounds().
     */
    double LoadedBounds[6];
    double LoadedBoundsAA[6];
    double VolumeGeometry[24];
    int Extents[6];
  };

  vtkTypeMacro(vtkVolumeTexture, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   *  Set a number of blocks per axis.
   */
  void SetPartitions(int x, int y, int z);
  const Size3& GetPartitions();

  /**
   * Loads the data array into the texture in the case only a single block is
   * is defined. Does not load when the input data is divided in multiple blocks
   * (in which case they will be loaded into GPU memory by GetNextBlock()).
   * Requires an active OpenGL context.
   */
  bool LoadVolume(
    vtkRenderer* ren, vtkDataSet* data, vtkDataArray* scalars, int isCell, int interpolation);

  /**
   * It currently only calls SetInterpolation internally. Requires an active OpenGL
   * context.
   */
  void UpdateVolume(vtkVolumeProperty* property);

  /**
   * If streaming the data array as separate blocks, sort them back to front.
   * This method does nothing if there is a single block.
   */
  void SortBlocksBackToFront(vtkRenderer* ren, vtkMatrix4x4* volumeMat);

  /**
   * Return the next volume block to be rendered and load its data.  If the
   * current block is the last one, it will return nullptr.
   */
  VolumeBlock* GetNextBlock();
  /**
   * Return the currently loaded block.
   */
  VolumeBlock* GetCurrentBlock();

  /**
   * Clean-up acquired graphics resources.
   */
  void ReleaseGraphicsResources(vtkWindow* win);

  /**
   * Get the scale and bias values given a VTK scalar type and a finite range.
   * The scale and bias values computed using this method can be useful for
   * custom shader code. For example, when looking up color values through the
   * transfer function texture, the scalar value must be scaled and offset.
   */
  static void GetScaleAndBias(int scalarType, float* scalarRange, float& scale, float& bias);
  vtkDataArray* GetLoadedScalars();

  bool HandleLargeDataTypes;
  float Scale[4];
  float Bias[4];
  float ScalarRange[4][2];
  float CellSpacing[3];
  int InterpolationType;
  vtkTimeStamp UploadTime;

  int IsCellData = 0;
  vtkNew<vtkMatrix4x4> CellToPointMatrix;
  float AdjustedTexMin[4];
  float AdjustedTexMax[4];

  vtkSmartPointer<vtkTextureObject> CoordsTex;

  int CoordsTexSizes[3];
  float CoordsScale[3];
  float CoordsBias[3];

  vtkSmartPointer<vtkTextureObject> BlankingTex;

protected:
  vtkVolumeTexture();
  ~vtkVolumeTexture() override;

private:
  vtkVolumeTexture(const vtkVolumeTexture&) = delete;
  void operator=(const vtkVolumeTexture&) = delete;

  /**
   * Load an image block as defined in volBlock into GPU memory.
   * Requires an active OpenGL context.
   */
  bool LoadTexture(int interpolation, VolumeBlock* volBlock);

  /**
   * Divide the image data in NxMxO user-defined blocks.
   */
  void SplitVolume(vtkImageData* imageData, Size3 const& part);

  void CreateBlocks(unsigned int format, unsigned int internalFormat, int type);

  void AdjustExtentForCell(Size6& extent);
  Size3 ComputeBlockSize(int* extent);

  /**
   * Defines OpenGL's texture type, format and internal format based on the
   * vtkDataArray type (scalarType) and the number of array components.
   */
  void SelectTextureFormat(unsigned int& format, unsigned int& internalFormat, int& type,
    int scalarType, int noOfComponents);

  /**
   * Clean-up any acquired host side resources (image blocks, etc.).
   */
  void ClearBlocks();

  /**
   * Computes loaded bounds in data-coordinates.
   */
  // can be combined into one call
  void ComputeBounds(VolumeBlock* block);
  void UpdateTextureToDataMatrix(VolumeBlock* block);

  /**
   *  Compute transformation from cell texture-coordinates to point texture-coords
   *  (CTP). Cell data maps correctly to OpenGL cells, point data does not (VTK
   *  defines points at the cell corners). To set the point data in the center of the
   *  OpenGL texels, a translation of 0.5 texels is applied, and the range is rescaled
   *  to the point range.
   *
   *  delta = TextureExtentsMax - TextureExtentsMin;
   *  min   = vec3(0.5) / delta;
   *  max   = (delta - vec3(0.5)) / delta;
   *  range = max - min
   *
   *  CTP = translation * Scale
   *  CTP = range.x,        0,        0,  min.x
   *              0,  range.y,        0,  min.y
   *              0,        0,  range.z,  min.z
   *              0,        0,        0,    1.0
   */
  void ComputeCellToPointMatrix(int extents[6]);

  ///@{
  /**
   * @brief Helper functions to catch potential issues when doing GPU
   * texture allocations.
   *
   * They make use of the available OpenGL mechanisms to try to detect whether
   * a volume would not fit in the GPU (due to MAX_TEXTURE_SIZE limitations,
   * memory availability, etc.).
   */
  bool AreDimensionsValid(vtkTextureObject* texture, int width, int height, int depth);

  bool SafeLoadTexture(vtkTextureObject* texture, int width, int height, int depth, int numComps,
    int dataType, void* dataPtr);
  ///@}

  void UpdateInterpolationType(int interpolation);
  void SetInterpolation(int interpolation);

  //----------------------------------------------------------------------------
  vtkTimeStamp UpdateTime;

  vtkSmartPointer<vtkTextureObject> Texture;
  std::vector<vtkDataSet*> ImageDataBlocks;
  std::map<vtkDataSet*, VolumeBlock*> ImageDataBlockMap;
  std::vector<VolumeBlock*> SortedVolumeBlocks;
  size_t CurrentBlockIdx;
  bool StreamBlocks;

  std::vector<Size3> TextureSizes;
  Size6 FullExtent;
  Size3 FullSize;
  Size3 Partitions;

  vtkDataArray* Scalars;
};

VTK_ABI_NAMESPACE_END
#endif // vtkVolumeTexture_h
