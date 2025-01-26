// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkCompositePolyDataMapperDelegator
 * @brief Delegates rendering of multiple polydata that share similar signatures.
 *
 * Concrete graphics implementations are expected to manage and trampoline
 * render functions to the delegate.
 *
 * @sa vtkOpenGLCompositePolyDataMapperDelegator vtkCompositePolyDataMapper
 */

#ifndef vtkCompositePolyDataMapperDelegator_h
#define vtkCompositePolyDataMapperDelegator_h

#include "vtkObject.h"

#include "vtkColor.h"               // for ivar
#include "vtkRenderingCoreModule.h" // for export macro
#include "vtkSmartPointer.h"        // for ivar
#include "vtkVector.h"              // for ivar

#include <memory> // for shared_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractMapper;
class vtkCompositePolyDataMapper;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkScalarsToColors;
class vtkImageData;

class VTKRENDERINGCORE_EXPORT vtkCompositePolyDataMapperDelegator : public vtkObject
{
public:
  static vtkCompositePolyDataMapperDelegator* New();
  vtkTypeMacro(vtkCompositePolyDataMapperDelegator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Shallow copies scalar array related properties into the delegate.
   */
  virtual void ShallowCopy(vtkCompositePolyDataMapper* polydataMapper);

  /**
   * Get a reference to the delegate.
   */
  vtkSmartPointer<vtkPolyDataMapper> GetDelegate() noexcept { return this->Delegate; }

  // This class encapsulates rendering attributes for a vtkPolyData
  struct BatchElement
  {
    bool Marked;
    bool IsOpaque;
    bool Visibility;
    bool Pickability;
    bool OverridesColor;
    bool ScalarVisibility;
    bool UseLookupTableScalarRange;
    bool InterpolateScalarsBeforeMapping;

    int ColorMode;
    int ScalarMode;
    int ArrayAccessMode;
    int ArrayComponent;
    int ArrayId;
    vtkIdType FieldDataTupleId;

    unsigned int FlatIndex;

    vtkColor3d AmbientColor;
    vtkColor3d DiffuseColor;
    vtkColor3d SpecularColor;
    vtkColor3d SelectionColor;

    double Opacity;
    double SelectionOpacity;
    vtkVector2d ScalarRange;

    std::string ArrayName;
    vtkScalarsToColors* LookupTable = nullptr;
    vtkPolyData* PolyData = nullptr;
  };

  ///@{
  /**
   * Keep track of what data is being used as the structure
   * can change
   */
  bool GetMarked() { return this->Marked; }
  void Mark() { this->Marked = true; }
  void Unmark()
  {
    this->Marked = false;
    this->UnmarkBatchElements();
  }
  virtual void ClearUnmarkedBatchElements() = 0;
  virtual void UnmarkBatchElements() = 0;
  ///@}

  /**
   * Accessor to the ordered list of PolyData that we last drew.
   */
  virtual std::vector<vtkPolyData*> GetRenderedList() const = 0;

  /**
   * Assign a parent mapper. The parent enables delegates to access
   * higher level attributes.
   *
   * Delegates can access attributes like ColorMissingArraysWithNanColor
   * and selection accessed. Also, they can invoke events on the parent mapper.
   * Ex: UpdateShaderEvent
   */
  virtual void SetParent(vtkCompositePolyDataMapper* mapper) = 0;

  /**
   * Add input polydata and it's rendering attributes to internal storage.
   */
  virtual void Insert(BatchElement&& element) = 0;

  /**
   * Get the batch element that describes attributes for a vtkPolyData.
   */
  virtual BatchElement* Get(vtkPolyData* polydata) = 0;

  /**
   * Clear all batch elements.
   */
  virtual void Clear() = 0;

protected:
  vtkCompositePolyDataMapperDelegator();
  ~vtkCompositePolyDataMapperDelegator() override;

  vtkSmartPointer<vtkPolyDataMapper> Delegate;

  bool Marked = false;

private:
  vtkCompositePolyDataMapperDelegator(const vtkCompositePolyDataMapperDelegator&) = delete;
  void operator=(const vtkCompositePolyDataMapperDelegator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
