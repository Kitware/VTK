/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeTexture.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class  vtkVolumeTexture
 * @brief  Creates and manages the volume texture rendered by
 * vtkOpenGLGPUVolumeRayCastMapper.
 *
 * Wraps a vtkTextureObject for which it selects the appropriate format (depending
 * on the input vtkDataArray type, number of components, etc.) and loads input data.
 * The class maintains a set of members of interest to the parent mapper, such as:
 *
 * * Active vtkDataArray scalar range.
 * * Volume's scale and bias (pixel transfer functions).
 * * HandleLargeDataType flag.
 *
 * This class supports streaming the volume data in separate blocks to make it fit
 * in graphics memory (sometimes referred to as bricking). The data is split into
 * a user-defined number of blocks in such a way that a single sub-block (brick) fits
 * completely into GPU memory.  A stride is passed to OpenGL so that it can access
 * the underlying vtkDataArray adequately for each of the blocks to be streamed
 * into GPU memory (back-to-front for correct composition).
 * Streaming the volume as separate texture bricks certainly imposes a performance
 * trade-off but acts as a graphics memory expansion scheme for devices that would
 * not be able to render higher resoulution volumes otherwise.
 *
 * @warning There are certain caveats when texture streaming is enabled, given the
 * locality constraint that rendering a single block imposes.
 *
 * - Quality might suffer near the block seams with ShadeOn() (gradient computation at
 * the boundaries needs adjustment).
 *
 * - Not all of the features supported by the mapper currently work correctly. This
 * is a list of known issues:
 *   -# Volume masks are currently not being streamed. They need to use a vtkVolumeTexture
 *      object instead. This will be fixed in a future commit.
 *   -# Blending modes such as average and additive might compute a different value near
 *      the edges.
 *
 *  - Future work will extend the API to be able to compute an ideal number of
 *  partitions and extents based on the platform capabilities.
 *
 * @warning This is an internal class of vtkOpenGLGPUVolumeRayCastMapper. It assumes
 * there is an active OpenGL context in each of its methods involving GL calls
 * (MakeCurrent() is expected to be called in the mapper before any member calls
 * involving gl functions).
 */


#ifndef vtkVolumeTexture_h
#define vtkVolumeTexture_h

#include <map>                               // For ImageDataBlockMap
#include <vector>                            // For ImageDataBlocks

#include "vtkRenderingVolumeOpenGL2Module.h" // For export macro
#include "vtkObject.h"
#include "vtkTimeStamp.h"                    // For UploadTime
#include "vtkTuple.h"                        // For Size6 and Size3


class vtkDataArray;
class vtkImageData;
class vtkOpenGLGPUVolumeRayCastMapper;
class vtkRenderer;
class vtkTextureObject;

class VTKRENDERINGVOLUMEOPENGL2_EXPORT vtkVolumeTexture : public vtkObject
{
  typedef vtkTuple<int, 6> Size6;
  typedef vtkTuple<int, 3> Size3;

public:
  static vtkVolumeTexture* New();

  struct VolumeBlock
  {
    VolumeBlock(vtkImageData* imData, vtkTextureObject* tex, Size3 const& texSize)
    {
      // Block extent is stored in vtkImageData
      ImageData = imData;
      TextureObject = tex;
      TextureSize = texSize;
      TupleIndex = 0;
    }

    vtkImageData* ImageData;
    vtkTextureObject* TextureObject;
    Size3 TextureSize;
    vtkIdType TupleIndex;
  };

  vtkTypeMacro(vtkVolumeTexture, vtkObject);
  void PrintSelf( ostream& os, vtkIndent indent ) VTK_OVERRIDE;

  /**
   * Set the parent volume mapper and initialize internals.
   */
  void SetMapper(vtkOpenGLGPUVolumeRayCastMapper* mapper);

  /**
   *  Set a number of blocks per axis.
   */
  void SetPartitions(int const x, int const y, int const z);

  /**
   * Loads the data array into the texture in the case only a single block is
   * is defined. Does not load when the input data is divided in multiple blocks
   * (in which case they will be loaded into GPU memory by GetNextBlock()).
   * Requires an active OpenGL context.
   */
  bool LoadVolume(vtkRenderer* ren, vtkImageData* data, vtkDataArray* scalars,
    int const interpolation);

  void UpdateInterpolationType(int const interpolation);

  /**
   * If streaming the data array as separate blocks, sort them back to front.
   * This method does nothing if there is a single block.
   */
  void SortBlocksBackToFront(vtkRenderer *ren, vtkMatrix4x4* volumeMat);

  /**
   * Return the next volume block to be rendered. When streaming is active (number
   * of blocks > 1), this method will upload and release graphics resources of
   * each block.
   */
  VolumeBlock* GetNextBlock();

  /**
   * Clean-up acquired graphics resources.
   */
  void ReleaseGraphicsResources(vtkWindow* win);

  //----------------------------------------------------------------------------

  bool HandleLargeDataTypes;
  float Scale[4];
  float Bias[4];
  double ScalarRange[4][2];
  int InterpolationType;
  vtkTimeStamp UploadTime;

protected:
  vtkVolumeTexture();
  ~vtkVolumeTexture() VTK_OVERRIDE;

private:
  /**
   * Load an image block as defined in volBlock into GPU memory.
   * Requires an active OpenGL context.
   */
  bool LoadTexture(int const interpolation, VolumeBlock* volBlock);

  /**
   * Divide the image data in NxMxO user-defined blocks.
   */
  void SplitVolume(vtkImageData* imageData, Size3 const & part);

  /**
   * Requires an active OpenGL context.
   */
  void SetInterpolation(int const interpolation);

  void CreateBlocks(unsigned int const format, unsigned int const internalFormat,
    int const type);

  void AdjustExtentForCell(Size6& extent);
  Size3 ComputeBlockSize(int* extent);

  /**
   * Defines OpenGL's texture type, format and internal format based on the
   * vtkDataArray type (scalarType) and the number of array components.
   */
  void SelectTextureFormat(unsigned int& format, unsigned int& internalFormat,
    int& type, int const scalarType, int const noOfComponents);

  /**
   * Clean-up any acquired host side resources (image blocks, etc.).
   */
  void ClearBlocks();

  vtkVolumeTexture(const vtkVolumeTexture&) VTK_DELETE_FUNCTION;
  void operator=(const vtkVolumeTexture&) VTK_DELETE_FUNCTION;

  //@{
  /**
   * @brief Helper functions to catch potential issues when doing GPU
   * texture allocations.
   *
   * They make use of the available OpenGL mechanisms to try to detect whether
   * a volume would not fit in the GPU (due to MAX_TEXTURE_SIZE limitations,
   * memory availability, etc.).
   */
  bool AreDimensionsValid(vtkTextureObject* texture, int const width,
    int const height, int const depth);

  bool SafeLoadTexture(vtkTextureObject* texture, int const width,
    int const height, int const depth, int numComps, int dataType, void* dataPtr);
  //@}

  //----------------------------------------------------------------------------
  vtkTextureObject* Texture;
  std::vector<vtkImageData*> ImageDataBlocks;
  std::map<vtkImageData*, VolumeBlock*> ImageDataBlockMap;
  std::vector<VolumeBlock*> SortedVolumeBlocks;
  size_t CurrentBlockIdx;
  bool StreamBlocks;

  std::vector<Size3> TextureSizes;
  Size6 FullExtent;
  Size3 FullSize;
  Size3 FullSizeAdjusted; /* Cell Adjusted */
  Size3 Partitions;

  vtkDataArray* Scalars;
  vtkOpenGLGPUVolumeRayCastMapper* Mapper;
};

#endif //vtkVolumeTexture_h
