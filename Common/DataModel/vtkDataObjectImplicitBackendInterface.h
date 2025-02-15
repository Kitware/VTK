// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkDataObjectImplicitBackendInterface_h
#define vtkDataObjectImplicitBackendInterface_h

#include "vtkCommonDataModelModule.h"

#include "vtkAOSDataArrayTemplate.h"
#include "vtkDataArrayMeta.h"
#include "vtkSmartPointer.h" // for internal member
#include "vtkWeakPointer.h"  // for internal member

/**
 * \struct vtkDataObjectImplicitBackendInterface
 * \brief A utility structure that can be used as a base for implicit array backend that relies
 * on a vtkDataObject.
 *
 * \details Some backend need to use the vtkDataObject itself to compute
 * the array value. This leads to a circular dependency, as the array is itself contained
 * inside the vtkDataObject. To break this dependency and to avoid issues with memory management,
 * those backends may inherits from this vtkDataObjectImplicitBackendInterface.
 *
 * vtkDataObjectImplicitBackendInterface observes the DeleteEvent of the vtkDataObject.
 * When raised, the backend instantiates the whole array in memory before the
 * actual dataobject deletion.
 *
 * The constructor requires an ArrayName and its Association in order to retrieve
 * the whole array to initialize the cache.
 *
 * # Inheritance
 * When inheriting from vtkDataObjectImplicitBackendInterface, `operator()`
 * should not be defined by the child class.
 * Only `GetValueFromDataObject()` should be overridden to return the expected value.
 *
 * Once the vtkDataObject is deleted, the parent class handle the memory allocation
 * and initialization. Then `operator()` uses the allocated memory instead
 * of calling the custom GetValueFromDataObject().
 *
 * Example of usage from vtkCountFaces:
 * \code{.cpp}
 * class vtkNumberOfFacesBackend : public vtkDataObjectImplicitBackendInterface<vtkIdType>
 * {
 *  vtkNumberOfFacesBackend(vtkDataObject* input, const std::string& name, int type)
 *    : vtkDataObjectImplicitBackendInterface(input, name, type)
 *  {}
 *
 * ~vtkNumberOfFacesBackend() override = default;
 *
 * vtkIdType GetValueFromDataObject(const vtkIdType index) const override
 * {
 *   return this->DataObject->GetCell(index)->GetNumberOfFaces();
 * }
 * };
 * \endcode
 *
 */
VTK_ABI_NAMESPACE_BEGIN

class vtkDataObject;
class vtkObject;

template <typename ValueType>
struct VTKCOMMONDATAMODEL_EXPORT vtkDataObjectImplicitBackendInterface
{
  vtkDataObjectImplicitBackendInterface(
    vtkDataObject* dataobject, const std::string& arrayName, int attributeType);

  virtual ~vtkDataObjectImplicitBackendInterface();

  /**
   * Get the value at given index. Entry point for vtkImplicitArray to uses the backend.
   *
   * This delegates to GetValueFromDataObject when DataObject is not nullptr,
   * and uses a local Cache otherwise.
   * Should not be reimplemented by subclasses.
   */
  ValueType operator()(vtkIdType idx) const;

protected:
  /**
   * Callback to call when DataObject is destroyed.
   * Internally call InitializeCache.
   */
  void OnDataObjectDeleted(vtkObject* caller, unsigned long eventId, void* calldata);

  /**
   * Return the actual value for given index.
   * Should be reimplemented by any subclass.
   */
  virtual ValueType GetValueFromDataObject(vtkIdType idx) const = 0;

  /**
   * Get the internal dataobject.
   */
  vtkDataObject* GetDataObject() { return this->DataObject; }

private:
  /**
   * Return the vtkImplicitArray made from this backend.
   * Useful to create the internal cache.
   */
  vtkDataArray* GetArray();

  vtkWeakPointer<vtkDataObject> DataObject;

  vtkSmartPointer<vtkAOSDataArrayTemplate<ValueType>> Cache;
  std::string ArrayName;
  int AttributeType;
};

VTK_ABI_NAMESPACE_END
#endif

#if defined(VTK_DATAOBJECT_BACKEND_INSTANTIATING)

#define VTK_INSTANTIATE_DATAOBJECT_BACKEND(ValueType)                                              \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template struct VTKCOMMONDATAMODEL_EXPORT vtkDataObjectImplicitBackendInterface<ValueType>;      \
  VTK_ABI_NAMESPACE_END

#elif defined(VTK_USE_EXTERN_TEMPLATE)

#ifndef VTK_DATAOBJECT_BACKEND_TEMPLATE_EXTERN
#define VTK_DATAOBJECT_BACKEND_TEMPLATE_EXTERN
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4910) // extern and dllexport incompatible
#endif
VTK_ABI_NAMESPACE_BEGIN
vtkExternTemplateMacro(
  extern template struct VTKCOMMONDATAMODEL_EXPORT vtkDataObjectImplicitBackendInterface);
VTK_ABI_NAMESPACE_END
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif // VTK_DATAOBJECT_IMPLICIT_BACKEND_TEMPLATE_EXTERN

#endif
