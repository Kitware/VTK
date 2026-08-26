// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGPUVolumeRayCastMapperInternals
 * @brief   Per input device state of vtkWebGPUVolumeRayCastMapper
 *
 * The mapper prepares its own inputs and nothing else: it tabulates the transfer functions and
 * classifies each port into cells that say where the volume is empty and where it is detailed.
 * What it does not do is upload or draw. The prepared inputs are offered to the
 * vtkWebGPUVolumeRayCastCoordinator of the renderer, which decides which bricks are worth keeping
 * on the device and marches them together with the volumes of every other mapper in the scene.
 * Voxels leave the host only when the coordinator asks for a brick, through the
 * vtkWebGPUVolumeBrickSource that each input is.
 *
 * Everything that names a WebGPU type lives here so that the public header of the mapper stays
 * free of the WebGPU headers.
 */

#ifndef vtkWebGPUVolumeRayCastMapperInternals_h
#define vtkWebGPUVolumeRayCastMapperInternals_h

#include "vtkWebGPUVolumeRayCastMapper.h"

#include "Private/vtkWebGPUVolumeRayCastContribution.h"

#include "vtkMatrix4x4.h" // for the per input index to physical transform
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkTimeStamp.h"
#include "vtk_wgpu.h"

#include <array>
#include <cstdint>
#include <map>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

class vtkDataArray;
class vtkImageData;
class vtkVolume;
class vtkVolumeProperty;
class vtkWebGPURenderWindow;
class vtkWebGPUVolumeRayCastCoordinator;

class vtkWebGPUVolumeRayCastMapper::vtkInternals
{
public:
  static constexpr int TableSize = vtkWebGPUVolumeTransferFunctionSize;

  explicit vtkInternals(vtkWebGPUVolumeRayCastMapper* self)
    : Self(self)
  {
  }

  /**
   * Bring every connected input up to date on the device and offer it to \a coordinator. Called
   * in the SyncDeviceResources stage, before anything is drawn.
   */
  void SyncDeviceResources(vtkWebGPURenderWindow* window,
    vtkWebGPUVolumeRayCastCoordinator* coordinator, vtkVolume* volume);

  void ReleaseGraphicsResources();

  /// Per port appearance overrides, used when a port is not backed by its own vtkVolume.
  std::map<int, vtkSmartPointer<vtkVolumeProperty>> PortProperties;

private:
  struct Input : public vtkWebGPUVolumeBrickSource
  {
    int Port = 0;
    vtkImageData* Data = nullptr;
    vtkVolumeProperty* Property = nullptr;
    vtkMatrix4x4* WorldMatrix = nullptr;

    bool FetchBrick(const int brick[3], int slotSize, void* voxels) override;
    bool FetchMask(const int brick[3], int slotSize, unsigned char* voxels) override;

    /// One byte per voxel, non zero where the voxel is not to be drawn. Empty when nothing is.
    std::vector<unsigned char> Hidden;
    /// Whether hiding a voxel should also hide what interpolates through it.
    bool MaskHidesNeighbours = false;

    /// The array the mapper was told to render, which is not always the active point scalars.
    vtkDataArray* Scalars = nullptr;
    bool CellData = false;
    /// Index space to the input's own coordinates, shifted by half a voxel for cell data so that
    /// a cell is sampled at its centre.
    vtkNew<vtkMatrix4x4> IndexToPhysical;

    /// Whether the scalars go to the device as bytes rather than floats.
    bool BytePool = false;

    int Dimensions[3] = { 0, 0, 0 };
    float ValueScale = 1.0f;
    float ValueBias = 0.0f;

    /// How many components of the array are rendered, at most four, and whether each carries its
    /// own transfer functions or they describe one colour between them.
    int NumberOfComponents = 1;
    bool IndependentComponents = true;

    double DataRange[4][2] = { { 0.0, 1.0 }, { 0.0, 1.0 }, { 0.0, 1.0 }, { 0.0, 1.0 } };
    /// Range each component's colour and opacity functions are tabulated over.
    double ScalarRange[4][2] = { { 0.0, 1.0 }, { 0.0, 1.0 }, { 0.0, 1.0 }, { 0.0, 1.0 } };
    /// Range each component's gradient opacity function is tabulated over.
    double GradientRange[4][2] = { { 0.0, 1.0 }, { 0.0, 1.0 }, { 0.0, 1.0 }, { 0.0, 1.0 } };

    /// Geometry of the acceleration cells, and the scalar extremes of each, per component.
    int CellDimensions[3] = { 0, 0, 0 };
    int CellSize = 0;
    std::vector<float> CellMinimum[4];
    std::vector<float> CellMaximum[4];
    /// Two floats per cell: largest opacity the transfer functions map it to, and how much it
    /// varies. Rebuilt whenever the transfer functions change.
    std::vector<float> CellClassification;

    /// RGBA16Float texels ready to be dropped into a layer of the shared arrays, one set of
    /// tables per component laid out one after another.
    std::vector<std::uint16_t> ColorTable;
    std::vector<std::uint16_t> OpacityTable;
    std::vector<std::uint16_t> GradientTable;
    /// Every component's opacity table again, unpacked, so cells can be classified without
    /// asking the transfer functions again.
    std::array<std::array<float, TableSize>, 4> OpacityCurve = {};
    /// Which component that is: the one the intensity modes and the classification look at.
    int PrimaryComponent = 0;

    vtkMTimeType DataTime = 0;
    vtkMTimeType TransferFunctionTime = 0;
    /// Bumped whenever anything above is rebuilt.
    vtkTimeStamp ContentTime;
  };

  bool CollectInputs(vtkVolume* volume);
  void UpdateVolumeData(Input& input, int cellSize);
  void UpdateHiddenVoxels(Input& input);
  void UpdateTransferFunctions(Input& input);
  void UpdateCellClassification(Input& input);

  vtkWebGPUVolumeRayCastMapper* Self = nullptr;

  /// Device state of every port that has ever been rendered, keyed by port.
  std::map<int, Input> InputCache;
  /// Ports offered this frame.
  std::vector<int> ActivePorts;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkWebGPUVolumeRayCastMapperInternals.h
