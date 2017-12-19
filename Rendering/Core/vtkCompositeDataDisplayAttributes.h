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
 * @brief   Rendering attributes for a multi-block dataset.
 *
 * The vtkCompositeDataDisplayAttributes class stores display attributes
 * for individual blocks in a multi-block dataset. It uses the actual data
 * block's pointer as a key (vtkDataObject*).
 *
 * @warning It is considered unsafe to dereference key pointers at any time,
 * they should only serve as keys to access the internal map.
*/

#ifndef vtkCompositeDataDisplayAttributes_h
#define vtkCompositeDataDisplayAttributes_h
#include <functional>               // for std::function
#include <unordered_map>            // for std::unordered_map

#include "vtkColor.h"               // for vtkColor3d
#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // for export macro


class vtkBoundingBox;
class vtkDataObject;

class VTKRENDERINGCORE_EXPORT vtkCompositeDataDisplayAttributes : public vtkObject
{
public:
  static vtkCompositeDataDisplayAttributes* New();
  vtkTypeMacro(vtkCompositeDataDisplayAttributes, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns true if any block has any block visibility is set.
   */
  bool HasBlockVisibilities() const;

  //@{
  /**
   * Set/get the visibility for the block with \p data_object.
   */
  void SetBlockVisibility(vtkDataObject* data_object, bool visible);
  bool GetBlockVisibility(vtkDataObject* data_object) const;
  //@}

  /**
   * Returns true if the block with the given data_object has a visibility
   * set.
   */
  bool HasBlockVisibility(vtkDataObject* data_object) const;

  /**
   * Removes the block visibility flag for the block with data_object.
   */
  void RemoveBlockVisibility(vtkDataObject* data_object);

  /**
   * Removes all block visibility flags. This effectively sets the visibility
   * for all blocks to true.
   */
  void RemoveBlockVisibilities();
  // This method is deprecated and will be removed in VTK 8.2. It is misspelled.
  VTK_LEGACY(void RemoveBlockVisibilites());

  /**
   * Returns true if any block has any block pickability is set.
   */
  bool HasBlockPickabilities() const;

  //@{
  /**
   * Set/get the pickability for the block with \p data_object.
   */
  void SetBlockPickability(vtkDataObject* data_object, bool visible);
  bool GetBlockPickability(vtkDataObject* data_object) const;
  //@}

  /**
   * Returns true if the block with the given data_object has a pickability
   * set.
   */
  bool HasBlockPickability(vtkDataObject* data_object) const;

  /**
   * Removes the block pickability flag for the block with data_object.
   */
  void RemoveBlockPickability(vtkDataObject* data_object);

  /**
   * Removes all block pickability flags. This effectively sets the pickability
   * for all blocks to true.
   */
  void RemoveBlockPickabilities();

  //@{
  /**
   * Set/get the color for the block with \p data_object.
   */
  void SetBlockColor(vtkDataObject* data_object, const double color[3]);
  void GetBlockColor(vtkDataObject* data_object, double color[3]) const;
  vtkColor3d GetBlockColor(vtkDataObject* data_object) const;
  //@}

  /**
   * Returns true if any block has any block color is set.
   */
  bool HasBlockColors() const;

  /**
   * Returns true if the block with the given \p data_object has a color.
   */
  bool HasBlockColor(vtkDataObject* data_object) const;

  /**
   * Removes the block color for the block with \p data_object.
   */
  void RemoveBlockColor(vtkDataObject* data_object);

  /**
   * Removes all block colors.
   */
  void RemoveBlockColors();

  //@{
  /**
   * Set/get the opacity for the block with data_object.
   */
  void SetBlockOpacity(vtkDataObject* data_object, double opacity);
  double GetBlockOpacity(vtkDataObject* data_object) const;
  //@}

  /**
   * Returns true if any block has an opacity set.
   */
  bool HasBlockOpacities() const;

  /**
   * Returns true if the block with data_object has an opacity set.
   */
  bool HasBlockOpacity(vtkDataObject* data_object) const;

  /**
   * Removes the set opacity for the block with data_object.
   */
  void RemoveBlockOpacity(vtkDataObject* data_object);

  /**
   * Removes all block opacities.
   */
  void RemoveBlockOpacities();

  //@{
  /**
   * Set/get the material for the block with data_object.
   * Only rendering backends that support advanced materials need to respect these.
   */
  void SetBlockMaterial(vtkDataObject* data_object, const std::string& material);
  const std::string& GetBlockMaterial(vtkDataObject* data_object) const;
  //@}

  /**
   * Returns true if any block has an material set.
   */
  bool HasBlockMaterials() const;

  /**
   * Returns true if the block with data_object has an material set.
   */
  bool HasBlockMaterial(vtkDataObject* data_object) const;

  /**
   * Removes the set material for the block with data_object.
   */
  void RemoveBlockMaterial(vtkDataObject* data_object);

  /**
   * Removes all block materialss.
   */
  void RemoveBlockMaterials();

  /**
   * If the input \a dobj is a vtkCompositeDataSet, we will loop over the
   * hierarchy recursively starting from initial index 0 and use only visible
   * blocks, which is specified in the vtkCompositeDataDisplayAttributes \a cda,
   * to compute the \a bounds.
   */
  static void ComputeVisibleBounds(
    vtkCompositeDataDisplayAttributes* cda,
    vtkDataObject *dobj,
    double bounds[6]);

  /**
   * Get the DataObject corresponding to the node with index flat_index under
   * parent_obj. Traverses the entire hierarchy recursively.
   */
  static vtkDataObject* DataObjectFromIndex(const unsigned int flat_index,
    vtkDataObject* parent_obj, unsigned int& current_flat_index);

  void VisitVisibilities(std::function<bool(vtkDataObject*, bool)> visitor)
  {
    for (auto entry : this->BlockVisibilities)
    {
      if (visitor(entry.first, entry.second))
      {
        break;
      }
    }
  }
protected:
  vtkCompositeDataDisplayAttributes();
  ~vtkCompositeDataDisplayAttributes() override;

private:
  vtkCompositeDataDisplayAttributes(const vtkCompositeDataDisplayAttributes&) = delete;
  void operator=(const vtkCompositeDataDisplayAttributes&) = delete;

  /**
   * If the input data \a dobj is a vtkCompositeDataSet, we will
   * loop over the hierarchy recursively starting from the initial block
   * and use only visible blocks, which is specified in the
   * vtkCompositeDataDisplayAttributes \a cda, to compute bounds and the
   * result bounds will be set to the vtkBoundingBox \a bbox. The \a parentVisible
   * is the visibility for the starting block.
   */
  static void ComputeVisibleBoundsInternal(
    vtkCompositeDataDisplayAttributes* cda,
    vtkDataObject *dobj,
    vtkBoundingBox* bbox,
    bool parentVisible = true);

  using BoolMap = std::unordered_map<vtkDataObject*, bool>;
  using DoubleMap = std::unordered_map<vtkDataObject*, double>;
  using ColorMap = std::unordered_map<vtkDataObject*, vtkColor3d>;
  using StringMap = std::unordered_map<vtkDataObject*, std::string>;

  BoolMap BlockVisibilities;
  ColorMap BlockColors;
  DoubleMap BlockOpacities;
  StringMap BlockMaterials;
  BoolMap BlockPickabilities;
};

#endif // vtkCompositeDataDisplayAttributes_h
