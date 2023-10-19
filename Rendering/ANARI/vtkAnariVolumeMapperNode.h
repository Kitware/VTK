/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAnariVolumeMapperNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
   * Make ANARI calls to render me.
   */
  virtual void Render(bool prepass) override;

protected:
  vtkAnariVolumeMapperNode();
  ~vtkAnariVolumeMapperNode();

  int ColorSize;
  int OpacitySize;
  vtkAnariVolumeMapperNodeInternals* Internal;

private:
  vtkAnariVolumeMapperNode(const vtkAnariVolumeMapperNode&) = delete;
  void operator=(const vtkAnariVolumeMapperNode&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
