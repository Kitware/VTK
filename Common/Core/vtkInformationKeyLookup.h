// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkInformationKeyLookup
 * @brief   Find vtkInformationKeys from name and
 * location strings.
 */

#ifndef vtkInformationKeyLookup_h
#define vtkInformationKeyLookup_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

#include <string> // For std::string

VTK_ABI_NAMESPACE_BEGIN
class vtkInformationKey;
class vtkCommonInformationKeyManager;
class vtkFilteringInformationKeyManager;

class VTKCOMMONCORE_EXPORT vtkInformationKeyLookup : public vtkObject
{
public:
  static vtkInformationKeyLookup* New();
  vtkTypeMacro(vtkInformationKeyLookup, vtkObject);

  /**
   * Lists all known keys.
   */
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Find an information key from name and location strings. For example,
   * Find("GUI_HIDE", "vtkAbstractArray") returns vtkAbstractArray::GUI_HIDE.
   * Note that this class only knows about keys in modules that are currently
   * linked to the running executable.
   */
  static vtkInformationKey* Find(const std::string& name, const std::string& location);

  /**
   * Find an information key by name alone. Returns the key if exactly one
   * key with the given name exists (regardless of location). Returns nullptr
   * in two distinct cases: no key matches, or multiple keys match (the
   * name is ambiguous across different locations). Callers that need to
   * disambiguate should use Find(name, location) instead.
   */
  static vtkInformationKey* FindByName(const std::string& name);

protected:
  vtkInformationKeyLookup();
  ~vtkInformationKeyLookup() override;

  friend class vtkInformationKey;
  friend class vtkCommonInformationKeyManager;
  friend class vtkFilteringInformationKeyManager;

  /**
   * Reference-count the lookup table so it remains valid until every
   * vtkInformationKey manager has finished its ClassFinalize.  Each
   * manager calls RetainCleanup in ClassInitialize and ReleaseCleanup
   * in ClassFinalize.  When the count drops to zero the underlying
   * storage is destroyed.  Restricted via friend access to the key
   * managers so external callers cannot unbalance the reference count.
   */
  static void RetainCleanup();
  static void ReleaseCleanup();

  /**
   * Add a key to the KeyMap. This is done automatically in the
   * vtkInformationKey constructor.
   */
  static void RegisterKey(
    vtkInformationKey* key, const std::string& name, const std::string& location);

  /**
   * Remove a key from the KeyMap. This is done automatically in the
   * vtkInformationKey destructor to avoid leaving a dangling pointer
   * in the lookup table when a key is destroyed before static
   * finalization (e.g. when Python releases a key created via MakeKey).
   */
  static void UnregisterKey(vtkInformationKey* key);

private:
  vtkInformationKeyLookup(const vtkInformationKeyLookup&) = delete;
  void operator=(const vtkInformationKeyLookup&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkInformationKeyLookup_h
