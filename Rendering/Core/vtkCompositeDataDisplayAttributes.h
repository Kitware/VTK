/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataDisplayAttributes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompositeDataDisplayAttributes - rendering attributes for a
// multi-block dataset.
// .SECTION Description
// The vtkCompositeDataDisplayAttributes class stores display attributes
// for individual blocks in a multi-block dataset.

#ifndef __vtkCompositeDataDisplayAttributes_h
#define __vtkCompositeDataDisplayAttributes_h

#include "vtkRenderingCoreModule.h" // for export macro
#include "vtkObject.h"
#include "vtkColor.h" // for vtkColor3d

#include <map> // for std::map

class VTKRENDERINGCORE_EXPORT vtkCompositeDataDisplayAttributes : public vtkObject
{
public:
  static vtkCompositeDataDisplayAttributes* New();
  vtkTypeMacro(vtkCompositeDataDisplayAttributes, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns true if any block has any block visibility is set.
  bool HasBlockVisibilities() const;

  // Description:
  // Set/get the visibility for the block with \p flat_index.
  void SetBlockVisibility(unsigned int flat_index, bool visible);
  bool GetBlockVisibility(unsigned int flat_index) const;

  // Description:
  // Returns true if the block with the given flat_index has a visiblity
  // set.
  bool HasBlockVisibility(unsigned int flat_index) const;

  // Description:
  // Removes the block visibility flag for the block with flat_index.
  void RemoveBlockVisibility(unsigned int flat_index);

  // Description:
  // Removes all block visibility flags. The effectively sets the visibility
  // for all blocks to true.
  void RemoveBlockVisibilites();

  // Description:
  // Set/get the color for the block with \p flat_index.
  void SetBlockColor(unsigned int flat_index, const double color[3]);
  void GetBlockColor(unsigned int flat_index, double color[3]) const;
  vtkColor3d GetBlockColor(unsigned int flat_index) const;

  // Description:
  // Returns true if any block has any block color is set.
  bool HasBlockColors() const;

  // Description:
  // Returns true if the block with the given \p flat_index has a color.
  bool HasBlockColor(unsigned int flat_index) const;

  // Description:
  // Removes the block color for the block with \p flat_index.
  void RemoveBlockColor(unsigned int flat_index);

  // Description:
  // Removes all block colors.
  void RemoveBlockColors();

  // Description:
  // Set/get the opacity for the block with flat_index.
  void SetBlockOpacity(unsigned int flat_index, double opacity);
  double GetBlockOpacity(unsigned int flat_index) const;

  // Description:
  // Returns true if any block has an opacity set.
  bool HasBlockOpacities() const;

  // Description:
  // Returns true if the block with flat_index has an opacity set.
  bool HasBlockOpacity(unsigned int flat_index) const;

  // Description:
  // Removes the set opacity for the block with flat_index.
  void RemoveBlockOpacity(unsigned int flat_index);

  // Description:
  // Removes all block opacities.
  void RemoveBlockOpacities();

protected:
  vtkCompositeDataDisplayAttributes();
  ~vtkCompositeDataDisplayAttributes();

private:
  vtkCompositeDataDisplayAttributes(const vtkCompositeDataDisplayAttributes&); // Not implemented.
  void operator=(const vtkCompositeDataDisplayAttributes&); // Not implemented.

private:
  std::map<unsigned int, bool> BlockVisibilities;
  std::map<unsigned int, vtkColor3d> BlockColors;
  std::map<unsigned int, double> BlockOpacities;
};

#endif // __vtkCompositeDataDisplayAttributes_h
