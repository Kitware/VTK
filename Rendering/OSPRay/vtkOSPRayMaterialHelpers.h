/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayMaterial.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOSPRayMaterialHelpers
 * @brief   convert vtk appearance controls to ospray materials
 *
 * Routines that convert vtk's appearance controlling state into ospray
 * specific calls to create materials. The key piece of information is the
 * vtkProperty::MaterialName, the rest is looked up from the
 * vtkOSPRayMaterialLibrary singleton.
 * The routines here are used by vtkOSPRayPolyDataMapperNode at render time.
 *
 * The contents here are private implementation details, and not meant to
 * be part of VTK's public API.
 *
 * @sa vtkOSPRayMaterialLibrary
*/

#ifndef vtkOSPRayMaterialHelpers_h
#define vtkOSPRayMaterialHelpers_h

#include <map>

#include "ospray/ospray.h" // for ospray handle types

class vtkImageData;
class vtkOSPRayRendererNode;

namespace vtkOSPRayMaterialHelpers {

  /**
   * Manufacture an ospray texture from a 2d vtkImageData
   */
  OSPTexture2D VTKToOSPTexture(vtkImageData *vColorTextureMap);

  /**
   * Construct a set of ospray materials for all of the material names.
   */
  void MakeMaterials(vtkOSPRayRendererNode *orn,
                     OSPRenderer oRenderer,
                     std::map<std::string, OSPMaterial> &mats);

  /**
   * Construct one ospray material within the given renderer that
   * corresponds to the visual characteristics set out in the named
   * material in the material library.
   */
  OSPMaterial MakeMaterial(vtkOSPRayRendererNode *orn,
                           OSPRenderer oRenderer,
                           std::string nickname);

  /**
   * Wraps ospNewMaterial or ospNewMaterial2, depending on OSPRay version.
   */
  OSPMaterial NewMaterial(vtkOSPRayRendererNode *orn,
                          OSPRenderer oRenderer,
                          std::string ospMatName);

}
#endif
// VTK-HeaderTest-Exclude: vtkOSPRayMaterialHelpers.h
