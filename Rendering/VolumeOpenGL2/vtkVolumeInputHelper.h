/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeInputHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkVolumeInputHelper
 * @brief Convenience container for internal structures specific to a volume
 * input.
 *
 * This class stores together vtkVolumeTexture, Lookup tables internal
 * and other input specific parameters. It also provides convenience methods
 * to manage the lookup tables.
 *
 * @warning This is an internal class of vtkOpenGLGPUVolumeRayCastMapper. It
 * assumes there is an active OpenGL context in methods involving GL calls
 * (MakeCurrent() is expected to be called in the mapper beforehand).
 *
 * \sa vtkOpenGLGPUVolumeRayCastMapper
 */
#ifndef vtkVolumeInputHelper_h
#define vtkVolumeInputHelper_h
#ifndef __VTK_WRAP__
#include <map>

#include "vtkSmartPointer.h"                 // For SmartPointer
#include "vtkTimeStamp.h"                    // For TimeStamp


class vtkOpenGLVolumeGradientOpacityTables;
class vtkOpenGLVolumeOpacityTables;
class vtkOpenGLVolumeRGBTables;
class vtkOpenGLTransferFunctions2D;
class vtkRenderer;
class vtkShaderProgram;
class vtkVolume;
class vtkVolumeTexture;
class vtkWindow;

class vtkVolumeInputHelper
{
public:
  vtkVolumeInputHelper() = default;
  vtkVolumeInputHelper(vtkSmartPointer<vtkVolumeTexture> tex, vtkVolume* vol);

  void RefreshTransferFunction(vtkRenderer* ren, const int uniformIndex,
    const int blendMode, const float samplingDist);
  void ForceTransferInit();

  void ActivateTransferFunction(vtkShaderProgram* prog, const int blendMode);
  void DeactivateTransferFunction(const int blendMode);

  void ReleaseGraphicsResources(vtkWindow* window);

  vtkSmartPointer<vtkVolumeTexture> Texture;
  vtkVolume* Volume = nullptr;

  /**
   * Defines the various component modes supported by
   * vtkGPUVolumeRayCastMapper.
   */
  enum ComponentMode {
    INVALID = 0,
    INDEPENDENT = 1,
    LA = 2,
    RGBA = 4
  };
  int ComponentMode = INDEPENDENT;

  /**
   * Transfer function internal structures and helpers.
   */
  vtkSmartPointer<vtkOpenGLVolumeGradientOpacityTables> GradientOpacityTables;
  vtkSmartPointer<vtkOpenGLVolumeOpacityTables> OpacityTables;
  vtkSmartPointer<vtkOpenGLVolumeRGBTables> RGBTables;
  vtkSmartPointer<vtkOpenGLTransferFunctions2D> TransferFunctions2D;

  /**
   * Maps uniform texture variable names to its corresponding texture unit.
   */
  std::map<int, std::string> RGBTablesMap;
  std::map<int, std::string> OpacityTablesMap;
  std::map<int, std::string> GradientOpacityTablesMap;
  std::map<int, std::string> TransferFunctions2DMap;

  /**
   * These values currently stored in vtkGPUVolumeRCMapper but should be moved
   * into vtkVolumeProperty in order to store them closer to the relevant
   * transfer functions and separately for each input.
   */
  int ColorRangeType = 0;           // vtkGPUVolumeRayCastMapper::SCALAR
  int ScalarOpacityRangeType = 0;   // vtkGPUVolumeRayCastMapper::SCALAR
  int GradientOpacityRangeType = 0; // vtkGPUVolumeRayCastMapper::SCALAR

  /**
   * Stores the uniform variable name where the gradient will be stored for
   * this input in the fragment shader.
   */
  std::string GradientCacheName;

protected:
  void InitializeTransferFunction(vtkRenderer* ren, const int index);

  void CreateTransferFunction1D(vtkRenderer* ren, const int index);
  void CreateTransferFunction2D(vtkRenderer* ren, const int index);

  void UpdateTransferFunctions(vtkRenderer* ren, const int blendMode,
    const float samplingDist);
  int UpdateOpacityTransferFunction(vtkRenderer* ren, vtkVolume* vol,
    unsigned int component, const int blendMode, const float samplingDist);
  int UpdateColorTransferFunction(vtkRenderer* ren, vtkVolume* vol,
    unsigned int component);
  int UpdateGradientOpacityTransferFunction(vtkRenderer* ren, vtkVolume* vol,
    unsigned int component, const float samplingDist);
  void UpdateTransferFunction2D(vtkRenderer* ren, unsigned int component);

  void ReleaseGraphicsTransfer1D(vtkWindow* window);
  void ReleaseGraphicsTransfer2D(vtkWindow* window);

  vtkTimeStamp LutInit;
  bool InitializeTransfer = true;
};

#endif
#endif // vtkVolumeInputHelper_h
// VTK-HeaderTest-Exclude: vtkVolumeInputHelper.h
