// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAnariVolumeMapperNode
 * @brief   links vtkVolumeMapper  to ANARI
 *
 * Translates vtkVolumeMapper state into ANARI rendering calls
 *
 * @par Thanks:
 * Kevin Griffin kgriffin@nvidia.com for creating and contributing the class
 * and NVIDIA for supporting this work.
 */

#ifndef vtkAnariVolumeMapperNode_h
#define vtkAnariVolumeMapperNode_h

#include "vtkRenderingAnariModule.h" // For export macro
#include "vtkVolumeMapperNode.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkAnariVolumeMapperNodeInternals;
class vtkVolume;

class VTKRENDERINGANARI_EXPORT vtkAnariVolumeMapperNode : public vtkVolumeMapperNode
{
public:
  static vtkAnariVolumeMapperNode* New();
  vtkTypeMacro(vtkAnariVolumeMapperNode, vtkVolumeMapperNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Convenience method to set/get color array size.
   */
  vtkSetMacro(ColorSize, int);
  vtkGetMacro(ColorSize, int);
  //@}

  //@{
  /**
   * Convenience method to set/get opacity array size.
   */
  vtkSetMacro(OpacitySize, int);
  vtkGetMacro(OpacitySize, int);
  //@}

  /**
   * Synchronize VTK and ANARI Objects
   */
  void Synchronize(bool prepass) override;
  /**
   * Add volume to ANARI world.
   */
  void Render(bool prepass) override;

private:
  vtkAnariVolumeMapperNode();
  ~vtkAnariVolumeMapperNode() override;

  vtkVolume* GetVtkVolume() const;
  bool VolumeWasModified() const;

  int ColorSize{ 128 };
  int OpacitySize{ 128 };
  vtkAnariVolumeMapperNodeInternals* Internal{ nullptr };

  vtkAnariVolumeMapperNode(const vtkAnariVolumeMapperNode&) = delete;
  void operator=(const vtkAnariVolumeMapperNode&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
