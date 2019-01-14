/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataDisplayAttributesLegacy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCompositeDataDisplayAttributesLegacy
 * @brief   rendering attributes for a
 * multi-block dataset.
 *
 * The vtkCompositeDataDisplayAttributesLegacy class stores display attributes
 * for individual blocks in a multi-block dataset. Attributes are mapped to
 * blocks through their flat-index; This is the mechanism used in legacy
 * OpenGL classes.
*/

#ifndef vtkCompositeDataDisplayAttributesLegacy_h
#define vtkCompositeDataDisplayAttributesLegacy_h

#include "vtkColor.h" // for vtkColor3d
#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // for export macro

#include <map> // for std::map

class vtkBoundingBox;
class vtkDataObject;

class VTKRENDERINGCORE_EXPORT vtkCompositeDataDisplayAttributesLegacy : public vtkObject
{
public:
  static vtkCompositeDataDisplayAttributesLegacy* New();
  vtkTypeMacro(vtkCompositeDataDisplayAttributesLegacy, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
   * Returns true if the block with the given flat_index has a visibility
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
  void RemoveBlockVisibilities();
  // This method is deprecated and will be removed in VTK 8.2. It is misspelled.
  VTK_LEGACY(void RemoveBlockVisibilites());

  /**
   * Returns true if any block has any block visibility is set.
   */
  bool HasBlockPickabilities() const;

  //@{
  /**
   * Set/get the visibility for the block with \p flat_index.
   */
  void SetBlockPickability(unsigned int flat_index, bool visible);
  bool GetBlockPickability(unsigned int flat_index) const;
  //@}

  /**
   * Returns true if the block with the given flat_index has a visibility
   * set.
   */
  bool HasBlockPickability(unsigned int flat_index) const;

  /**
   * Removes the block visibility flag for the block with flat_index.
   */
  void RemoveBlockPickability(unsigned int flat_index);

  /**
   * Removes all block visibility flags. The effectively sets the visibility
   * for all blocks to true.
   */
  void RemoveBlockPickabilities();

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
  // blocks, which is specified in the vtkCompositeDataDisplayAttributesLegacy \a cda,
  // to compute the \a bounds.
  static void ComputeVisibleBounds(
    vtkCompositeDataDisplayAttributesLegacy* cda,
    vtkDataObject *dobj,
    double bounds[6]);

protected:
  vtkCompositeDataDisplayAttributesLegacy();
  ~vtkCompositeDataDisplayAttributesLegacy() override;

private:
  vtkCompositeDataDisplayAttributesLegacy(const vtkCompositeDataDisplayAttributesLegacy&) = delete;
  void operator=(const vtkCompositeDataDisplayAttributesLegacy&) = delete;

  /**
   * If the input data \a dobj is a vtkCompositeDataSet, we will
   * loop over the hierarchy recursively starting from initial index
   * \a flat_index and use only visible blocks, which is
   * specified in the vtkCompositeDataDisplayAttributesLegacy \a cda,
   * to compute bounds and the result bounds will be set to
   * the vtkBoundingBox \a bbox. The \a paraentVisible is the
   * visibility for the starting flat_index.
   */
  static void ComputeVisibleBoundsInternal(
    vtkCompositeDataDisplayAttributesLegacy* cda,
    vtkDataObject *dobj,
    unsigned int& flat_index,
    vtkBoundingBox* bbox,
    bool parentVisible = true);

  std::map<unsigned int, bool> BlockVisibilities;
  std::map<unsigned int, vtkColor3d> BlockColors;
  std::map<unsigned int, double> BlockOpacities;
  std::map<unsigned int, bool> BlockPickabilities;

};

#endif // vtkCompositeDataDisplayAttributesLegacy_h
