// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkObject.h"

#include "vtkColor.h"               // for vtkColor3d
#include "vtkRenderingCoreModule.h" // for export macro
#include "vtkSmartPointer.h"        // for arg
#include "vtkVector.h"              // for vtkVector2d
#include "vtkWrappingHints.h"       // For VTK_MARSHALMANUAL

#include <functional>    // for std::function
#include <unordered_map> // for std::unordered_map

VTK_ABI_NAMESPACE_BEGIN
class vtkBoundingBox;
class vtkDataObject;
class vtkDeserializer;
class vtkScalarsToColors;
class vtkSerializer;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALMANUAL vtkCompositeDataDisplayAttributes : public vtkObject
{
public:
  static vtkCompositeDataDisplayAttributes* New();
  vtkTypeMacro(vtkCompositeDataDisplayAttributes, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns true if any block has any block visibility is set.
   */
  bool HasBlockVisibilities() const;

  ///@{
  /**
   * Set/get the visibility for the block with \p data_object.
   */
  void SetBlockVisibility(vtkDataObject* data_object, bool visible);
  bool GetBlockVisibility(vtkDataObject* data_object) const;
  ///@}

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

  /**
   * Returns true if any block has any block pickability is set.
   */
  bool HasBlockPickabilities() const;

  ///@{
  /**
   * Set/get the pickability for the block with \p data_object.
   */
  void SetBlockPickability(vtkDataObject* data_object, bool visible);
  bool GetBlockPickability(vtkDataObject* data_object) const;
  ///@}

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

  ///@{
  /**
   * @see vtkMapper::SetScalarVisibility
   */
  void SetBlockScalarVisibility(vtkDataObject* data_object, bool value);
  bool GetBlockScalarVisibility(vtkDataObject* data_object) const;
  bool HasBlockScalarVisibility(vtkDataObject* data_object) const;
  bool HasBlockScalarVisibilities() const;
  void RemoveBlockScalarVisibility(vtkDataObject* data_object);
  void RemoveBlockScalarVisibilities();
  ///@}

  ///@{
  /**
   * @see vtkMapper::SetUseLookupTableScalarRange
   */
  void SetBlockUseLookupTableScalarRange(vtkDataObject* data_object, bool value);
  bool GetBlockUseLookupTableScalarRange(vtkDataObject* data_object) const;
  bool HasBlockUseLookupTableScalarRange(vtkDataObject* data_object) const;
  bool HasBlockUseLookupTableScalarRanges() const;
  void RemoveBlockUseLookupTableScalarRange(vtkDataObject* data_object);
  void RemoveBlockUseLookupTableScalarRanges();
  ///@}

  ///@{
  /**
   * @see vtkMapper::SetInterpolateScalarsBeforeMapping
   */
  void SetBlockInterpolateScalarsBeforeMapping(vtkDataObject* data_object, bool value);
  bool GetBlockInterpolateScalarsBeforeMapping(vtkDataObject* data_object) const;
  bool HasBlockInterpolateScalarsBeforeMapping(vtkDataObject* data_object) const;
  bool HasBlockInterpolateScalarsBeforeMappings() const;
  void RemoveBlockInterpolateScalarsBeforeMapping(vtkDataObject* data_object);
  void RemoveBlockInterpolateScalarsBeforeMappings();
  ///@}

  ///@{
  /**
   * Set/get the color for the block with \p data_object.
   */
  void SetBlockColor(vtkDataObject* data_object, const double color[3]);
  void GetBlockColor(vtkDataObject* data_object, double color[3]) const;
  vtkColor3d GetBlockColor(vtkDataObject* data_object) const;
  ///@}

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

  ///@{
  /**
   * Set/get the opacity for the block with data_object.
   */
  void SetBlockOpacity(vtkDataObject* data_object, double opacity);
  double GetBlockOpacity(vtkDataObject* data_object) const;
  ///@}

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

  ///@{
  /**
   * Set/get the material for the block with data_object.
   * Only rendering backends that support advanced materials need to respect these.
   */
  void SetBlockMaterial(vtkDataObject* data_object, const std::string& material);
  const std::string& GetBlockMaterial(vtkDataObject* data_object) const;
  ///@}

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

  ///@{
  /**
   * @see vtkMapper::SetColorMode
   */
  void SetBlockColorMode(vtkDataObject* data_object, int value);
  int GetBlockColorMode(vtkDataObject* data_object) const;
  bool HasBlockColorMode(vtkDataObject* data_object) const;
  bool HasBlockColorModes() const;
  void RemoveBlockColorMode(vtkDataObject* data_object);
  void RemoveBlockColorModes();
  ///@}

  ///@{
  /**
   * @see vtkMapper::SetScalarMode
   */
  void SetBlockScalarMode(vtkDataObject* data_object, int value);
  int GetBlockScalarMode(vtkDataObject* data_object) const;
  bool HasBlockScalarMode(vtkDataObject* data_object) const;
  bool HasBlockScalarModes() const;
  void RemoveBlockScalarMode(vtkDataObject* data_object);
  void RemoveBlockScalarModes();
  ///@}

  ///@{
  /**
   * @see vtkMapper::SetArrayAccessMode
   */
  void SetBlockArrayAccessMode(vtkDataObject* data_object, int value);
  int GetBlockArrayAccessMode(vtkDataObject* data_object) const;
  bool HasBlockArrayAccessMode(vtkDataObject* data_object) const;
  bool HasBlockArrayAccessModes() const;
  void RemoveBlockArrayAccessMode(vtkDataObject* data_object);
  void RemoveBlockArrayAccessModes();
  ///@}

  ///@{
  /**
   * @see vtkMapper::SetArrayComponent
   */
  void SetBlockArrayComponent(vtkDataObject* data_object, int value);
  int GetBlockArrayComponent(vtkDataObject* data_object) const;
  bool HasBlockArrayComponent(vtkDataObject* data_object) const;
  bool HasBlockArrayComponents() const;
  void RemoveBlockArrayComponent(vtkDataObject* data_object);
  void RemoveBlockArrayComponents();
  ///@}

  ///@{
  /**
   * @see vtkMapper::SetArrayId
   */
  void SetBlockArrayId(vtkDataObject* data_object, int value);
  int GetBlockArrayId(vtkDataObject* data_object) const;
  bool HasBlockArrayId(vtkDataObject* data_object) const;
  bool HasBlockArrayIds() const;
  void RemoveBlockArrayId(vtkDataObject* data_object);
  void RemoveBlockArrayIds();
  ///@}

  ///@{
  /**
   * @see vtkMapper::SetScalarRange
   */
  void SetBlockScalarRange(vtkDataObject* data_object, const vtkVector2d& value);
  vtkVector2d GetBlockScalarRange(vtkDataObject* data_object) const;
  bool HasBlockScalarRange(vtkDataObject* data_object) const;
  bool HasBlockScalarRanges() const;
  void RemoveBlockScalarRange(vtkDataObject* data_object);
  void RemoveBlockScalarRanges();
  ///@}

  ///@{
  /**
   * @see vtkMapper::SetArrayName
   */
  void SetBlockArrayName(vtkDataObject* data_object, const std::string& value);
  std::string GetBlockArrayName(vtkDataObject* data_object) const;
  bool HasBlockArrayName(vtkDataObject* data_object) const;
  bool HasBlockArrayNames() const;
  void RemoveBlockArrayName(vtkDataObject* data_object);
  void RemoveBlockArrayNames();
  ///@}

  ///@{
  /**
   * @see vtkMapper::SetFieldDataTupleId
   */
  void SetBlockFieldDataTupleId(vtkDataObject* data_object, vtkIdType value);
  vtkIdType GetBlockFieldDataTupleId(vtkDataObject* data_object) const;
  bool HasBlockFieldDataTupleId(vtkDataObject* data_object) const;
  bool HasBlockFieldDataTupleIds() const;
  void RemoveBlockFieldDataTupleId(vtkDataObject* data_object);
  void RemoveBlockFieldDataTupleIds();
  ///@}

  ///@{
  /**
   * @see vtkMapper::SetLookupTable
   */
  void SetBlockLookupTable(vtkDataObject* data_object, vtkSmartPointer<vtkScalarsToColors> lut);
  vtkSmartPointer<vtkScalarsToColors> GetBlockLookupTable(vtkDataObject* data_object) const;
  bool HasBlockLookupTable(vtkDataObject* data_object) const;
  bool HasBlockLookupTables() const;
  void RemoveBlockLookupTable(vtkDataObject* data_object);
  void RemoveBlockLookupTables();
  ///@}

  /**
   * If the input \a dobj is a vtkCompositeDataSet, we will loop over the
   * hierarchy recursively starting from initial index 0 and use only visible
   * blocks, which is specified in the vtkCompositeDataDisplayAttributes \a cda,
   * to compute the \a bounds.
   */
  static void ComputeVisibleBounds(
    vtkCompositeDataDisplayAttributes* cda, vtkDataObject* dobj, double bounds[6]);

  /**
   * Get the DataObject corresponding to the node with index flat_index under
   * parent_obj. Traverses the entire hierarchy recursively.
   */
  static vtkDataObject* DataObjectFromIndex(
    unsigned int flat_index, vtkDataObject* parent_obj, unsigned int current_flat_index = 0);

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
  static void ComputeVisibleBoundsInternal(vtkCompositeDataDisplayAttributes* cda,
    vtkDataObject* dobj, vtkBoundingBox* bbox, bool parentVisible = true);

  using BoolMap = std::unordered_map<vtkDataObject*, bool>;
  using DoubleMap = std::unordered_map<vtkDataObject*, double>;
  using IntMap = std::unordered_map<vtkDataObject*, int>;
  using VtkIdTypeMap = std::unordered_map<vtkDataObject*, vtkIdType>;
  using ColorMap = std::unordered_map<vtkDataObject*, vtkColor3d>;
  using RangeMap = std::unordered_map<vtkDataObject*, vtkVector2d>;
  using StringMap = std::unordered_map<vtkDataObject*, std::string>;
  using LookupTableMap = std::unordered_map<vtkDataObject*, vtkSmartPointer<vtkScalarsToColors>>;

  BoolMap BlockVisibilities;
  ColorMap BlockColors;
  DoubleMap BlockOpacities;
  StringMap BlockMaterials;
  BoolMap BlockPickabilities;
  BoolMap BlockScalarVisibilities;
  BoolMap BlockUseLookupTableScalarRanges;
  BoolMap BlockInterpolateScalarsBeforeMappings;
  IntMap BlockColorModes;
  IntMap BlockScalarModes;
  IntMap BlockArrayAccessModes;
  IntMap BlockArrayComponents;
  IntMap BlockArrayIds;
  RangeMap BlockScalarRanges;
  StringMap BlockArrayNames;
  LookupTableMap BlockLookupTables;
  VtkIdTypeMap BlockFieldDataTupleIds;

  friend class vtkCompositeDataDisplayAttributesSerDesHelper;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCompositeDataDisplayAttributes_h
