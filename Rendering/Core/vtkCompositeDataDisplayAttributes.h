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
/**
 * @class   vtkCompositeDataDisplayAttributes
 * @brief   rendering attributes for a
 * multi-block dataset.
 *
 * The vtkCompositeDataDisplayAttributes class stores display attributes
 * for individual blocks in a multi-block dataset.
*/

#ifndef vtkCompositeDataDisplayAttributes_h
#define vtkCompositeDataDisplayAttributes_h

#include "vtkRenderingCoreModule.h" // for export macro
#include "vtkObject.h"
#include "vtkColor.h" // for vtkColor3d

#include <map> // for std::map

class vtkBoundingBox;
class vtkDataObject;

class VTKRENDERINGCORE_EXPORT vtkCompositeDataDisplayAttributes : public vtkObject
{
public:
  static vtkCompositeDataDisplayAttributes* New();
  vtkTypeMacro(vtkCompositeDataDisplayAttributes, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Returns true if any block has any block visibility is set.
   */
  bool HasBlockVisibilities() const;

  //@{
  /**
   * Set/get the visibility for the block with \p flat_index.
   */
  void SetBlockVisibility(unsigned int flat_index, bool visible);
  bool GetBlockVisibility(unsigned int flat_index) const;
  //@}

  /**
   * Returns true if the block with the given flat_index has a visiblity
   * set.
   */
  bool HasBlockVisibility(unsigned int flat_index) const;

  /**
   * Removes the block visibility flag for the block with flat_index.
   */
  void RemoveBlockVisibility(unsigned int flat_index);

  /**
   * Removes all block visibility flags. The effectively sets the visibility
   * for all blocks to true.
   */
  void RemoveBlockVisibilites();

  //@{
  /**
   * Set/get the color for the block with \p flat_index.
   */
  void SetBlockColor(unsigned int flat_index, const double color[3]);
  void GetBlockColor(unsigned int flat_index, double color[3]) const;
  vtkColor3d GetBlockColor(unsigned int flat_index) const;
  //@}

  /**
   * Returns true if any block has any block color is set.
   */
  bool HasBlockColors() const;

  /**
   * Returns true if the block with the given \p flat_index has a color.
   */
  bool HasBlockColor(unsigned int flat_index) const;

  /**
   * Removes the block color for the block with \p flat_index.
   */
  void RemoveBlockColor(unsigned int flat_index);

  /**
   * Removes all block colors.
   */
  void RemoveBlockColors();

  //@{
  /**
   * Set/get the opacity for the block with flat_index.
   */
  void SetBlockOpacity(unsigned int flat_index, double opacity);
  double GetBlockOpacity(unsigned int flat_index) const;
  //@}

  /**
   * Returns true if any block has an opacity set.
   */
  bool HasBlockOpacities() const;

  /**
   * Returns true if the block with flat_index has an opacity set.
   */
  bool HasBlockOpacity(unsigned int flat_index) const;

  /**
   * Removes the set opacity for the block with flat_index.
   */
  void RemoveBlockOpacity(unsigned int flat_index);

  /**
   * Removes all block opacities.
   */
  void RemoveBlockOpacities();

  // If the input \a dobj is a vtkCompositeDataSet, we will loop over the
  // hierarchy recursively starting from initial index 0 and use only visible
  // blocks, which is specified in the vtkCompositeDataDisplayAttributes \a cda,
  // to compute the \a bounds.
  static void ComputeVisibleBounds(
    vtkCompositeDataDisplayAttributes* cda,
    vtkDataObject *dobj,
    double bounds[6]);

protected:
  vtkCompositeDataDisplayAttributes();
  ~vtkCompositeDataDisplayAttributes() VTK_OVERRIDE;

private:
  vtkCompositeDataDisplayAttributes(const vtkCompositeDataDisplayAttributes&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCompositeDataDisplayAttributes&) VTK_DELETE_FUNCTION;

  /**
   * If the input data \a dobj is a vtkCompositeDataSet, we will
   * loop over the hierarchy recursively starting from initial index
   * \a flat_index and use only visible blocks, which is
   * specified in the vtkCompositeDataDisplayAttributes \a cda,
   * to compute bounds and the result bounds will be set to
   * the vtkBoundingBox \a bbox. The \a paraentVisible is the
   * visibility for the starting flat_index.
   */
  static void ComputeVisibleBoundsInternal(
    vtkCompositeDataDisplayAttributes* cda,
    vtkDataObject *dobj,
    unsigned int& flat_index,
    vtkBoundingBox* bbox,
    bool parentVisible = true);

  std::map<unsigned int, bool> BlockVisibilities;
  std::map<unsigned int, vtkColor3d> BlockColors;
  std::map<unsigned int, double> BlockOpacities;

};

#endif // vtkCompositeDataDisplayAttributes_h
