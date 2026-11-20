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
#include "Private/vtkWebGPUVolumeRayCastMapperInput.h"

#include "vtkImageData.h" // for the stream shell
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

class vtkAlgorithm;
class vtkInformation;
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

  /**
   * What is known of a port that is read a brick at a time. Only pipeline information is needed to
   * decide, plus one voxel read to learn exactly which array the mapper takes and how it is stored.
   */
  struct Stream
  {
    bool Active = false;
    vtkAlgorithm* Source = nullptr;
    int SourcePort = 0;
    int WholeExtent[6] = { 0, -1, 0, -1, 0, -1 };
    /// The volume's geometry with no voxels in it, standing in for the data everywhere the whole
    /// volume would otherwise be asked about.
    vtkNew<vtkImageData> Shell;
    /// Type, components and name of the array read, and nothing else.
    vtkSmartPointer<vtkDataArray> Scalars;
    /// The range the source publishes for that array, when it does.
    bool HasRange = false;
    double Range[2] = { 0.0, 1.0 };
    vtkMTimeType PipelineTime = 0;
    /// The source could stream but what it produces cannot be, say because it is cell data. Held
    /// until something upstream changes, so the whole volume is asked for meanwhile.
    bool Refused = false;
  };

  /**
   * Whether \a port would be streamed, judged from its pipeline information alone. Safe to call
   * while a pipeline request is being answered, which UpdateStream is not.
   */
  bool WantsStreaming(int port, vtkInformation* info);

  /**
   * Whether \a port is streamed, bringing what is known of its source up to date. Asks the
   * pipeline for information only; the one voxel probe is repeated only when something upstream
   * changed.
   */
  bool UpdateStream(int port);
  std::map<int, Stream> Streams;

private:
  using Input = vtkWebGPUVolumeRayCastMapperInput;

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
