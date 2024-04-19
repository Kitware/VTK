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

#include <map>     // For std::map
#include <string>  // For std::string
#include <utility> // For std::pair

VTK_ABI_NAMESPACE_BEGIN
class vtkInformationKey;

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

protected:
  vtkInformationKeyLookup();
  ~vtkInformationKeyLookup() override;

  friend class vtkInformationKey;

  /**
   * Add a key to the KeyMap. This is done automatically in the
   * vtkInformationKey constructor.
   */
  static void RegisterKey(
    vtkInformationKey* key, const std::string& name, const std::string& location);

private:
  vtkInformationKeyLookup(const vtkInformationKeyLookup&) = delete;
  void operator=(const vtkInformationKeyLookup&) = delete;

  typedef std::pair<std::string, std::string> Identifier; // Location, Name
  typedef std::map<Identifier, vtkInformationKey*> KeyMap;

  // Using a static function / variable here to ensure static initialization
  // works as intended, since key objects are static, too.
  static KeyMap& Keys();
};

VTK_ABI_NAMESPACE_END
#endif // vtkInformationKeyLookup_h
