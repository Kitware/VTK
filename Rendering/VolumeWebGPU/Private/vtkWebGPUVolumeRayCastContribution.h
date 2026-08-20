// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @struct  vtkWebGPUVolumeRayCastContribution
 * @brief   One input offered up to be ray cast alongside the others
 *
 * A mapper fills one of these in per connected input and hands it to
 * vtkWebGPUVolumeRayCastCoordinator, which gathers the contributions of every volume prop in the
 * renderer and marches them together. Everything the coordinator needs to place, sample and shade
 * the input is here; the mapper keeps owning the memory, which stays valid for the frame.
 */

#ifndef vtkWebGPUVolumeRayCastContribution_h
#define vtkWebGPUVolumeRayCastContribution_h

#include "vtkABINamespace.h"
#include "vtkType.h"

#include <cstdint>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

class vtkImageData;
class vtkMatrix4x4;
class vtkPlaneCollection;
class vtkVolume;
class vtkVolumeProperty;
class vtkWebGPUVolumeRayCastMapper;

/// Size of every tabulated transfer function, in texels.
constexpr int vtkWebGPUVolumeTransferFunctionSize = 256;

/**
 * Where the coordinator gets the voxels of a brick from.
 *
 * The mapper never uploads a volume whole: it says how the volume is cut up and answers for one
 * brick at a time. An implementation may pull the brick through the VTK pipeline when asked, which
 * is what lets a volume that does not fit in memory, or arrives in pieces, work unchanged.
 */
struct vtkWebGPUVolumeBrickSource
{
  virtual ~vtkWebGPUVolumeBrickSource() = default;

  /**
   * Copy the brick at \a brick, with a one voxel apron from its neighbours, into \a voxels, which
   * holds \a slotSize cubed values in the contribution's pool kind, x fastest. Voxels outside the
   * volume repeat the nearest one. Returns false when the brick is not available yet, leaving it
   * out of this frame.
   */
  virtual bool FetchBrick(const int brick[3], int slotSize, void* voxels) = 0;
};

struct vtkWebGPUVolumeRayCastContribution
{
  /// Who offered this input, and which of its ports it came from.
  vtkWebGPUVolumeRayCastMapper* Mapper = nullptr;
  vtkVolume* Volume = nullptr;
  int Port = 0;

  vtkImageData* Data = nullptr;
  vtkVolumeProperty* Property = nullptr;
  /// Placement of the prop that carries this input.
  vtkMatrix4x4* WorldMatrix = nullptr;
  /// Clipping planes of the mapper that owns this input, if any.
  vtkPlaneCollection* ClippingPlanes = nullptr;

  /// Where the coordinator asks for voxels, and whether they come back as bytes or floats.
  vtkWebGPUVolumeBrickSource* BrickSource = nullptr;
  bool BytePool = false;

  int Dimensions[3] = { 0, 0, 0 };
  /// Turns a raw texel back into a scalar: scalar = texel * ValueScale + ValueBias.
  float ValueScale = 1.0f;
  float ValueBias = 0.0f;

  /// Range the colour and opacity functions were tabulated over.
  double ScalarRange[2] = { 0.0, 1.0 };
  /// Range the gradient opacity function was tabulated over.
  double GradientRange[2] = { 0.0, 1.0 };

  /// Edge length and count of the acceleration cells, which say where the volume is empty. Not the
  /// pool's bricks, which are the coordinator's business and usually much larger.
  int CellSize = 8;
  int CellDimensions[3] = { 0, 0, 0 };
  /// Two floats per cell: the largest opacity it maps to, and how much it varies.
  const std::vector<float>* CellClassification = nullptr;

  /// RGBA16Float texels, vtkWebGPUVolumeTransferFunctionSize of each.
  const std::vector<std::uint16_t>* ColorTable = nullptr;
  const std::vector<std::uint16_t>* OpacityTable = nullptr;
  const std::vector<std::uint16_t>* GradientTable = nullptr;

  /// Bumped whenever any of the data above is rebuilt, so the coordinator can leave the layers
  /// and blocks it already uploaded alone.
  vtkMTimeType ContentTime = 0;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkWebGPUVolumeRayCastContribution.h
