// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkDataArraySelection
 * @brief Store on/off settings for data arrays, etc.
 *
 * vtkDataArraySelection is intended to be used by algorithms that want to
 * expose a API that allow the user to enable/disable a collection of entities,
 * such as arrays. Readers, for example, can use vtkDataArraySelection to let
 * the user choose which array to read from the file.
 *
 * Originally intended for selecting data arrays (hence the name), this class
 * can be used for letting users choose other items too, for example,
 * vtkIOSSReader uses vtkDataArraySelection to let users choose
 * which blocks to read.
 *
 * Unlike most other vtkObject subclasses, vtkDataArraySelection has public API
 * that need not modify the MTime for the object. These M-Time non-modifying
 * methods are typically intended for use within the algorithm or reader to
 * populate the vtkDataArraySelection instance with available array names and
 * their default values.
 */

#ifndef vtkDataArraySelection_h
#define vtkDataArraySelection_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

#include <memory> // for std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkDataArraySelection : public vtkObject
{
public:
  vtkTypeMacro(vtkDataArraySelection, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkDataArraySelection* New();

  /**
   * Enable the array with the given name.  Creates a new entry if
   * none exists.
   *
   * This method will call `this->Modified()` if the enable state for the
   * array changed.
   */
  void EnableArray(const char* name);

  /**
   * Disable the array with the given name.  Creates a new entry if
   * none exists.
   *
   * This method will call `this->Modified()` if the enable state for the
   * array changed.
   */
  void DisableArray(const char* name);

  /**
   * Return whether the array with the given name is enabled.  If
   * there is no entry, the array is assumed to be disabled.
   */
  int ArrayIsEnabled(const char* name) const;

  /**
   * Return whether the array with the given name exists.
   */
  int ArrayExists(const char* name) const;

  /**
   * Enable all arrays that currently have an entry.
   *
   * This method will call `this->Modified()` if the enable state for any of the known
   * arrays is changed.
   */
  void EnableAllArrays();

  /**
   * Disable all arrays that currently have an entry.
   *
   * This method will call `this->Modified()` if the enable state for any of the known
   * arrays is changed.
   */
  void DisableAllArrays();

  /**
   * Get the number of arrays that currently have an entry.
   */
  int GetNumberOfArrays() const;

  /**
   * Get the number of arrays that are enabled.
   */
  int GetNumberOfArraysEnabled() const;

  /**
   * Get the name of the array entry at the given index.
   */
  const char* GetArrayName(int index) const;

  /**
   * Get an index of the array with the given name.
   */
  int GetArrayIndex(const char* name) const;

  /**
   * Get the index of an array with the given name among those
   * that are enabled.  Returns -1 if the array is not enabled.
   */
  int GetEnabledArrayIndex(const char* name) const;

  /**
   * Get whether the array at the given index is enabled.
   */
  int GetArraySetting(int index) const;

  /**
   * Get whether the array is enabled/disable using its name.
   */
  int GetArraySetting(const char* name) const { return this->ArrayIsEnabled(name); }

  /**
   * Set array setting given the name. If the array doesn't exist, it will be
   * added.
   *
   * This method will call `this->Modified()` if the enable state for the
   * array changed.
   */
  void SetArraySetting(const char* name, int setting);

  /**
   * Remove all array entries.
   *
   * This method will call `this->Modified()` if the arrays were cleared.
   */
  void RemoveAllArrays();

  /**
   * Add to the list of arrays that have entries.  For arrays that
   * already have entries, the settings are untouched.  For arrays
   * that don't already have an entry, they are assumed to be enabled
   * by default. The state can also be passed as the second argument.
   * This method should be called only by the filter owning this
   * object.
   *
   * This method **does not** call `this->Modified()`.
   *
   * Also note for arrays already known to this instance (i.e.
   * `this->ArrayExists(name) == true`, this method has no effect.
   */
  int AddArray(const char* name, bool state = true);

  /**
   * Remove an array setting given its index.
   *
   * This method **does not** call `this->Modified()`.
   */
  void RemoveArrayByIndex(int index);

  /**
   * Remove an array setting given its name.
   *
   * This method **does not** call `this->Modified()`.
   */
  void RemoveArrayByName(const char* name);

  ///@{
  /**
   * Set the list of arrays that have entries.  For arrays that
   * already have entries, the settings are copied.  For arrays that
   * don't already have an entry, they are assigned the given default
   * status.  If no default status is given, it is assumed to be on.
   * There will be no more entries than the names given.  This method
   * should be called only by the filter owning this object.  The
   * signature with the default must have a different name due to a
   * bug in the Borland C++ 5.5 compiler.
   *
   * This method **does not** call `this->Modified()`.
   */
  void SetArrays(const char* const* names, int numArrays);
  void SetArraysWithDefault(const char* const* names, int numArrays, int defaultStatus);
  ///@}

  /**
   * Copy the selections from the given vtkDataArraySelection instance.
   *
   * This method will call `this->Modified()` if the array selections changed.
   */
  void CopySelections(vtkDataArraySelection* selections);

  ///@{
  /**
   * Update `this` to include values from `other`. For arrays that don't
   * exist in `this` but exist in `other`, they will get added to `this` with
   * the same array setting as in `other`. Array settings for arrays already in
   * `this` are left unchanged.
   *
   * This method will call `this->Modified()` if the array selections changed
   * unless @a skipModified is set to true (default is false).
   */
  void Union(vtkDataArraySelection* other) { this->Union(other, false); }
  void Union(vtkDataArraySelection* other, bool skipModified);
  ///@}

  ///@{
  /**
   * Get/Set enabled state for any unknown arrays. Default is 0 i.e. not
   * enabled. When set to 1, `ArrayIsEnabled` will return 1 for any
   * array not explicitly specified.
   */
  vtkSetMacro(UnknownArraySetting, int);
  vtkGetMacro(UnknownArraySetting, int);
  ///@}

  /**
   * Copy contents of other. The MTime for this instance is modified only if
   * values are different.
   */
  void DeepCopy(const vtkDataArraySelection* other);

  /**
   * Returns true if the two array selections are equivalent.
   */
  bool IsEqual(const vtkDataArraySelection* other) const;

protected:
  vtkDataArraySelection();
  ~vtkDataArraySelection() override;

private:
  vtkDataArraySelection(const vtkDataArraySelection&) = delete;
  void operator=(const vtkDataArraySelection&) = delete;

  // Internal implementation details.
  class vtkInternals;
  std::unique_ptr<vtkInternals> Internal;
  int UnknownArraySetting;
};

VTK_ABI_NAMESPACE_END
#endif
