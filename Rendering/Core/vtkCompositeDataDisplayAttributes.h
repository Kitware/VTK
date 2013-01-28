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

#include <map> // for std::map

class VTKRENDERINGCORE_EXPORT vtkCompositeDataDisplayAttributes : public vtkObject
{
public:
  static vtkCompositeDataDisplayAttributes* New();
  vtkTypeMacro(vtkCompositeDataDisplayAttributes, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent);

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

protected:
  vtkCompositeDataDisplayAttributes();
  ~vtkCompositeDataDisplayAttributes();

private:
  vtkCompositeDataDisplayAttributes(const vtkCompositeDataDisplayAttributes&); // Not implemented.
  void operator=(const vtkCompositeDataDisplayAttributes&); // Not implemented.

private:
  std::map<unsigned int, bool> BlockVisibilities;
};

#endif // __vtkCompositeDataDisplayAttributes_h
