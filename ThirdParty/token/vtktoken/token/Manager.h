// © Kitware, Inc. See license.md for details.
#ifndef token_Manager_h
#define token_Manager_h

#include "token/Hash.h" // for Hash type-alias.
#include "token/Exports.h" // for export macros.

#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

token_BEGIN_NAMESPACE

/**
 * @class   Manager
 * @brief   Manage string-token mappings.
 *
 * The `Token` class holds an instance of this
 * class to map run-time hashes of strings back to the source
 * string.
 */

class TOKEN_EXPORT Manager : public std::enable_shared_from_this<Manager>
{
public:
  Manager() = default;
  virtual ~Manager() = default;

  virtual void printSelf(std::ostream& os, int indent);

  /// An enumerant visitors return to terminate early (or not).
  enum Visit
  {
    Halt,    //!< Terminate visitation.
    Continue //!< Continue visiting items.
  };

  /// Signature for functions visiting strings in the manager or in a set held by the manager.
  using Visitor = std::function<Visit(Hash entry)>;
  /// An invalid hash (that should never exist inside the manager's storage).
  /// This corresponds to the empty string:
  static inline constexpr Hash Invalid() { return 0x811c9dc5; }

  /// Insert a string into the manager by computing a unique hash (the returned value).
  Hash manage(const std::string& ss);
  /// Remove a hash from the manager. This also removes it from any string sets.
  /// The return value is the number of strings actually removed (0 or 1).
  std::size_t unmanage(Hash hh);

  /// Look up a string from its hashed value, \a hh.
  const std::string& value(Hash hh) const;
  /// Look up a hash from a string value (without inserting it).
  /// If the string has not been previously managed, then Manager::Invalid() will be returned.
  Hash find(const std::string& s) const;
  /// Compute a hash from a string value (without inserting it into the manager).
  /// If the string is not already managed, this will compute the hash value
  /// that *would* be used if the string were to be immediately inserted.
  /// This method allows hash collisions to be avoided; one can compute a hash while the
  /// map is write-locked and insert if needed.
  ///
  /// Unlike the \a find() method, this will never return Manager::Invalid().
  Hash compute(const std::string& ss) const;

  /// Add the hash \a hh to the set \a ss.
  ///
  /// The set \a ss need not exist prior to this call.
  /// It will be added to the manager as needed and
  /// then used as a key in the dictionary of sets.
  /// The returned value is the hash of the set \a ss (when passing a string for the set name)
  /// or a boolean indicating whether the insertion actually occurred (when passing a hash
  /// for the set name). Note that inserting an already-existing member will return false.
  Hash insert(const std::string& ss, Hash hh);
  bool insert(Hash ss, Hash hh);
  /// Remove the hash \a h from the set \a s.
  /// This returns true if the hash was removed and false otherwise (i.e., because
  /// the set did not exist or did not contain \a h.
  bool remove(const std::string& ss, Hash hh);
  bool remove(Hash ss, Hash hh);

  /**\brief Return true if the set \a ss exists and contains hash \a hh ; and false otherwise.
   *
   * If \a ss is Invalid(), then this returns true if the hash exists in \a Data
   * and false otherwise.
   */
  bool contains(const std::string& ss, Hash hh) const;
  bool contains(Hash ss, Hash hh) const;
  bool contains(Hash hh) const { return this->contains(Invalid(), hh); }

  /// Return true if the manager is empty (i.e., managing no hashes) and false otherwise.
  bool empty() const { return m_data.empty(); }

  /// Visit all members of the set (or the entire Manager if passed the Invalid() hash).
  ///
  /// Note that this method makes a copy of the set keys at the time it is invoked and
  /// visits them. Other threads (or even your \a visitor) may modify the string manager
  /// during traversal, so there is no guarantee when your \a visitor is invoked that
  /// the hash provided to it has a valid entry.
  ///
  /// You may terminate early by returning Visit::Halt.
  Visit visitMembers(Visitor visitor, Hash set = Invalid()) const;

  /// Visit all set names in the manager.
  ///
  /// Note that this method makes a copy of the set keys at the time it is invoked and
  /// visits them. Other threads (or even your \a visitor) may modify the string manager
  /// during traversal, so there is no guarantee when your \a visitor is invoked that
  /// the hash provided to it has a valid entry.
  ///
  /// You may terminate early by returning Visit::Halt.
  Visit visitSets(Visitor visitor) const;

  /// Reset the manager to an empty state, clearing both members and sets.
  void reset();

  /// Add a translation table entry for use during deserialization.
  ///
  /// If we are deserializing pre-hashed strings into a non-empty manager,
  /// it is possible that collisions will result in different numbers being
  /// assigned to strings depending on the order of insertion. This table
  /// can be used to identify translations (from the map's keys to the values)
  /// that should be applied when IDs from the deserialized manager are
  /// referenced by tokens also being deserialized.
  ///
  /// \a source is a serialized key value and \a target is its new value
  /// in this manager's m_data map.
  ///
  /// When you are finished deserializing, you should empty this table.
  void addTranslation(Hash source, Hash target);
  Hash getTranslation(Hash source) const;
  std::size_t resetTranslations();

private:
  using LockGuard = std::lock_guard<std::mutex>;
  /// Same as compute() but does not lock (you must hold m_writeLock upon entry).
  std::pair<Hash, bool> computeInternal(const std::string& s, const LockGuard& proofOfLock) const;
  std::pair<Hash, bool> computeInternalAndInsert(
    const std::string& s, const LockGuard& proofOfLock);
  std::size_t unmanageInternal(Hash hh, const LockGuard& proofOfLock);

  std::unordered_map<Hash, std::string> m_data;
  std::unordered_map<Hash, std::unordered_set<Hash>> m_sets;
  std::unordered_map<Hash, Hash> m_translation;
  mutable std::mutex m_writeLock;

  Manager(const Manager&) = delete;
  void operator=(const Manager&) = delete;
};

token_CLOSE_NAMESPACE
#endif // Manager_h
