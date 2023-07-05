// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkStringManager_h
#define vtkStringManager_h
/**
 * @class   vtkStringManager
 * @brief   Manage string-token mappings.
 *
 * The `vtkStringToken` class holds an instance of this
 * class to map run-time hashes of strings back to the source
 * string.
 */

#include "vtkObject.h"
#include "vtkStringToken.h" // for vtkStringToken::Hash type-alias.

#include <functional>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

VTK_ABI_NAMESPACE_BEGIN

class VTKCOMMONCORE_EXPORT vtkStringManager : public vtkObject
{
public:
  vtkTypeMacro(vtkStringManager, vtkObject);
  static vtkStringManager* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// An enumerant visitors return to terminate early (or not).
  enum Visit
  {
    Halt,    //!< Terminate visitation.
    Continue //!< Continue visiting items.
  };

  /// The type of integer used to hash strings.
  using Hash = std::uint32_t;

  /// Signature for functions visiting strings in the manager or in a set held by the manager.
  using Visitor = std::function<Visit(Hash entry)>;
  /// An invalid hash (that should never exist inside the manager's storage).
  static constexpr Hash Invalid = 0;

  /// Insert a string into the manager by computing a unique hash (the returned value).
  Hash Manage(const std::string& ss);
  /// Remove a hash from the manager. This also removes it from any string sets.
  /// The return value is the number of strings actually removed (0 or 1).
  std::size_t Unmanage(Hash hh);

  /// Look up a string from its hashed value, \a hh.
  const std::string& Value(Hash hh) const;
  /// Look up a hash from a string value (without inserting it).
  /// If the string has not been previously managed, then Manager::Invalid will be returned.
  Hash Find(const std::string& s) const;
  /// Compute a hash from a string value (without inserting it into the manager).
  /// If the string is not already managed, this will compute the hash value
  /// that *would* be used if the string were to be immediately inserted.
  /// This method allows hash collisions to be avoided; one can compute a hash while the
  /// map is write-locked and insert if needed.
  ///
  /// Unlike the \a find() method, this will never return Manager::Invalid.
  Hash Compute(const std::string& ss) const;

  /// Add the hash \a hh to the set \a ss.
  ///
  /// The set \a ss need not exist prior to this call.
  /// It will be added to the manager as needed and
  /// then used as a key in the dictionary of sets.
  /// The returned value is the hash of the set \a ss (when passing a string for the set name)
  /// or a boolean indicating whether the insertion actually occurred (when passing a hash
  /// for the set name). Note that inserting an already-existing member will return false.
  Hash Insert(const std::string& ss, Hash hh);
  bool Insert(Hash ss, Hash hh);
  /// Remove the hash \a h from the set \a s.
  /// This returns true if the hash was removed and false otherwise (i.e., because
  /// the set did not exist or did not contain \a h.
  bool Remove(const std::string& ss, Hash hh);
  bool Remove(Hash ss, Hash hh);

  /**\brief Return true if the set \a ss exists and contains hash \a hh ; and false otherwise.
   *
   * If \a ss is Invalid, then this returns true if the hash exists in \a Data
   * and false otherwise.
   */
  bool Contains(const std::string& ss, Hash hh) const;
  bool Contains(Hash ss, Hash hh) const;
  bool Contains(Hash hh) const { return this->Contains(Invalid, hh); }

  /// Return true if the manager is empty (i.e., managing no hashes) and false otherwise.
  bool Empty() const { return this->Data.empty(); }

  /// Visit all members of the set (or the entire Manager if passed the Invalid hash).
  ///
  /// Note that this method makes a copy of the set keys at the time it is invoked and
  /// visits them. Other threads (or even your \a visitor) may modify the string manager
  /// during traversal, so there is no guarantee when your \a visitor is invoked that
  /// the hash provided to it has a valid entry.
  ///
  /// You may terminate early by returning Visit::Halt.
  Visit VisitMembers(Visitor visitor, Hash set = Invalid) const;

  /// Visit all set names in the manager.
  ///
  /// Note that this method makes a copy of the set keys at the time it is invoked and
  /// visits them. Other threads (or even your \a visitor) may modify the string manager
  /// during traversal, so there is no guarantee when your \a visitor is invoked that
  /// the hash provided to it has a valid entry.
  ///
  /// You may terminate early by returning Visit::Halt.
  Visit VisitSets(Visitor visitor) const;

  /// Reset the manager to an empty state, clearing both members and sets.
  void Reset();

protected:
  vtkStringManager() = default;

private:
  using LockGuard = std::lock_guard<std::mutex>;
  /// Same as compute() but does not lock (you must hold WriteLock upon entry).
  std::pair<Hash, bool> ComputeInternal(const std::string& s, const LockGuard& proofOfLock) const;
  std::pair<Hash, bool> ComputeInternalAndInsert(
    const std::string& s, const LockGuard& proofOfLock);
  std::size_t UnmanageInternal(Hash hh, const LockGuard& proofOfLock);

  std::unordered_map<Hash, std::string> Data;
  std::unordered_map<Hash, std::unordered_set<Hash>> Sets;
  mutable std::mutex WriteLock;

  vtkStringManager(const vtkStringManager&) = delete;
  void operator=(const vtkStringManager&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkStringManager_h
