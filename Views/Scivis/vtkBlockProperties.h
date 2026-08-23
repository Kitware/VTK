// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkBlockProperties
 * @brief   Per-block appearance for a representation of composite data.
 *
 * vtkBlockProperties is the part of a representation that says how individual
 * blocks of a composite dataset are drawn, differently from the rest: whether a
 * block is drawn at all, and in what color and opacity.  A representation owns
 * one and hands it out through vtkSurfaceRepresentation::GetBlocks().
 *
 * @par Example usage:
 * @code
 * rep->GetBlocks()->SetVisibility(2, false);
 * rep->GetBlocks()->SetColor(3, 0.8, 0.2, 0.2);
 * rep->GetBlocks()->Reset();
 * @endcode
 *
 * @par
 * The same in Python:
 * @code
 * rep.blocks.SetVisibility(2, False)
 * rep.blocks.SetColor(3, 0.8, 0.2, 0.2)
 * @endcode
 *
 * Blocks are addressed by flat index, the same index composite dataset
 * iteration and the vtkCompositeIndex array use.  An index only means anything
 * against real data, because it has to be walked to find the block it names, so
 * these bring the representation up to date before resolving one and warn
 * rather than guess when there is nothing to resolve against.  That resolution
 * is the reason this is an object rather than a handful of methods: it is a
 * step every one of them would otherwise have to remember.
 *
 * A block with nothing set for it is drawn like the rest of the representation.
 * Reset() puts every block back to that.
 *
 * @sa vtkSurfaceRepresentation vtkCompositeDataDisplayAttributes
 */

#ifndef vtkBlockProperties_h
#define vtkBlockProperties_h

#include "vtkObject.h"
#include "vtkViewsScivisModule.h" // For export macro
#include "vtkWeakPointer.h"       // For ivars

VTK_ABI_NAMESPACE_BEGIN
class vtkAlgorithm;
class vtkCompositeDataDisplayAttributes;
class vtkCompositePolyDataMapper;

class VTKVIEWSSCIVIS_EXPORT vtkBlockProperties : public vtkObject
{
public:
  static vtkBlockProperties* New();
  vtkTypeMacro(vtkBlockProperties, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Whether the block at @a index is drawn.  A block nothing has been said
   * about is drawn, so this only ever needs setting to hide one.
   */
  void SetVisibility(unsigned int index, bool visible);
  bool GetVisibility(unsigned int index);
  ///@}

  ///@{
  /**
   * The color of the block at @a index, in place of the color the whole
   * representation is drawn in.  Has no effect while the representation is
   * coloring by an array.
   */
  void SetColor(unsigned int index, double r, double g, double b);
  void GetColor(unsigned int index, double color[3]);
  ///@}

  ///@{
  /**
   * How opaque the block at @a index is, in place of the representation's own
   * opacity.
   */
  void SetOpacity(unsigned int index, double opacity);
  double GetOpacity(unsigned int index);
  ///@}

  /**
   * Forget everything set per block, so that every block is drawn like the rest
   * of the representation again.
   */
  void Reset();

  /**
   * The attributes object these are stored in, for what is not covered here:
   * per-block textures, scalar mode and field data tuple ids.  Note that it is
   * keyed by data object rather than by index, which is the difference this
   * class exists to paper over.
   */
  vtkCompositeDataDisplayAttributes* GetDisplayAttributes();

  ///@{
  /**
   * The mapper these properties are applied to, and the representation that has
   * to be up to date before a flat index can be resolved against its data.
   *
   * Both are set by the representation that owns this object; there is no
   * reason for anything else to call them.  The representation is held weakly,
   * so an object kept alive after it has gone does nothing rather than follow a
   * dangling pointer.
   */
  void SetMapper(vtkCompositePolyDataMapper* mapper);
  void SetRepresentation(vtkAlgorithm* representation);
  ///@}

protected:
  vtkBlockProperties();
  ~vtkBlockProperties() override;

private:
  vtkBlockProperties(const vtkBlockProperties&) = delete;
  void operator=(const vtkBlockProperties&) = delete;

  /**
   * Bring the representation up to date so that a flat index can be resolved
   * against its data.  Returns false, with a warning, when there is nothing to
   * resolve against.
   */
  bool Prepare();

  vtkWeakPointer<vtkAlgorithm> Representation;
  vtkWeakPointer<vtkCompositePolyDataMapper> Mapper;
};

VTK_ABI_NAMESPACE_END
#endif
