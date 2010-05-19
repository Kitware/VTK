#ifndef __vtkExodusIICache_h
#define __vtkExodusIICache_h

// ============================================================================
// The following classes define an LRU cache for data arrays
// loaded by the Exodus reader. Here's how they work:
// 
// The actual cache consists of two STL containers: a set of
// cache entries (vtkExodusIICacheEntry) and a list of
// cache references (vtkExodusIICacheRef). The entries in
// these containers are sorted for fast retrieval:
// 1. The cache entries are indexed by the timestep, the
//    object type (edge block, face set, ...), and the
//    object ID (if one exists). When you call Find() to
//    retrieve a cache entry, you provide a key containing
//    this information and the array is returned if it exists.
// 2. The list of cache references are stored in "least-recently-used"
//    order. The least recently referenced array is the first in
//    the list. Whenever you request an entry with Find(), it is
//    moved to the back of the list if it exists.
// This makes retrieving arrays O(n log n) and popping LRU
// entries O(1). Each cache entry stores an iterator into
// the list of references so that it can be located quickly for
// removal.

#include "vtkObject.h"

#include <vtkstd/map> // used for cache storage
#include <vtkstd/list> // use for LRU ordering

//BTX
class VTK_HYBRID_EXPORT vtkExodusIICacheKey
{
public:
  int Time;
  int ObjectType;
  int ObjectId;
  int ArrayId;
  vtkExodusIICacheKey()
    {
    Time = -1;
    ObjectType = -1;
    ObjectId = -1;
    ArrayId = -1;
    }
  vtkExodusIICacheKey( int time, int objType, int objId, int arrId )
    {
    Time = time;
    ObjectType = objType;
    ObjectId = objId;
    ArrayId = arrId;
    }
  vtkExodusIICacheKey( const vtkExodusIICacheKey& src )
    {
    Time = src.Time;
    ObjectType = src.ObjectType;
    ObjectId = src.ObjectId;
    ArrayId = src.ArrayId;
    }
  bool match( const vtkExodusIICacheKey&other, const vtkExodusIICacheKey& pattern ) const
    {
    if ( pattern.Time && this->Time != other.Time )
      return false;
    if ( pattern.ObjectType && this->ObjectType != other.ObjectType )
      return false;
    if ( pattern.ObjectId && this->ObjectId != other.ObjectId )
      return false;
    if ( pattern.ArrayId && this->ArrayId != other.ArrayId )
      return false;
    return true;
    }
  bool operator < ( const vtkExodusIICacheKey& other ) const
    {
    if ( this->Time < other.Time )
      return true;
    else if ( this->Time > other.Time )
      return false;
    if ( this->ObjectType < other.ObjectType )
      return true;
    else if ( this->ObjectType > other.ObjectType )
      return false;
    if ( this->ObjectId < other.ObjectId )
      return true;
    else if ( this->ObjectId > other.ObjectId )
      return false;
    if ( this->ArrayId < other.ArrayId )
      return true;
    return false;
    }
};

class vtkExodusIICacheEntry;
class vtkExodusIICache;
class vtkDataArray;

typedef vtkstd::map<vtkExodusIICacheKey,vtkExodusIICacheEntry*> vtkExodusIICacheSet;
typedef vtkstd::map<vtkExodusIICacheKey,vtkExodusIICacheEntry*>::iterator vtkExodusIICacheRef;
typedef vtkstd::list<vtkExodusIICacheRef> vtkExodusIICacheLRU;
typedef vtkstd::list<vtkExodusIICacheRef>::iterator vtkExodusIICacheLRURef;

class VTK_HYBRID_EXPORT vtkExodusIICacheEntry
{
public:
  vtkExodusIICacheEntry();
  vtkExodusIICacheEntry( vtkDataArray* arr );
  vtkExodusIICacheEntry( const vtkExodusIICacheEntry& other );

  ~vtkExodusIICacheEntry();

  vtkDataArray* GetValue() { return this->Value; }

protected:
  vtkDataArray* Value;
  vtkExodusIICacheLRURef LRUEntry;

  friend class vtkExodusIICache;
};
//ETX

class VTK_HYBRID_EXPORT vtkExodusIICache : public vtkObject
{
public:
  static vtkExodusIICache* New();
  vtkTypeMacro(vtkExodusIICache,vtkObject);
  void PrintSelf( ostream& os, vtkIndent indent );

  /// Empty the cache
  void Clear();

  /// Set the maximum allowable cache size. This will remove cache entries if the capacity is reduced below the current size.
  void SetCacheCapacity( double sizeInMiB );

  /** See how much cache space is left.
    * This is the difference between the capacity and the size of the cache.
    * The result is in MiB.
    */
  double GetSpaceLeft()
    { return this->Capacity - this->Size; }

  /** Remove cache entries until the size of the cache is at or below the given size.
    * Returns a nonzero value if deletions were required.
    */
  int ReduceToSize( double newSize );

  //BTX
  /// Insert an entry into the cache (this can remove other cache entries to make space).
  void Insert( vtkExodusIICacheKey& key, vtkDataArray* value );

  /** Determine whether a cache entry exists. If it does, return it -- otherwise return NULL.
    * If a cache entry exists, it is marked as most recently used.
    */
  vtkDataArray*& Find( vtkExodusIICacheKey );

  /** Invalidate a cache entry (drop it from the cache) if the key exists.
    * This does nothing if the cache entry does not exist.
    * Returns 1 if the cache entry existed prior to this call and 0 otherwise.
    */
  int Invalidate( vtkExodusIICacheKey key );

  /** Invalidate all cache entries matching a specified pattern, dropping all matches from the cache.
    * Any nonzero entry in the \a pattern forces a comparison between the corresponding value of \a key.
    * Any cache entries satisfying all the comparisons will be dropped.
    * If pattern is entirely zero, this will empty the entire cache.
    * This is useful for invalidating all entries of a given object type.
    *
    * Returns the number of cache entries dropped.
    * It is not an error to specify an empty range -- 0 will be returned if one is given.
    */
  int Invalidate( vtkExodusIICacheKey key, vtkExodusIICacheKey pattern );
  //ETX

protected:
  /// Default constructor
  vtkExodusIICache();

  /// Destructor.
  ~vtkExodusIICache();


  /// Avoid (some) FP problems
  void RecomputeSize();

  /// The capacity of the cache (i.e., the maximum size of all arrays it contains) in MiB.
  double Capacity;

  /// The current size of the cache (i.e., the size of the all the arrays it currently contains) in MiB.
  double Size;

  //BTX
  /** A least-recently-used (LRU) cache to hold arrays.
    * During RequestData the cache may contain more than its maximum size since
    * the user may request more data than the cache can hold. However, the cache
    * is expunged whenever a new array is loaded. Never count on the cache holding
    * what you request for very long.
    */
  vtkExodusIICacheSet Cache;

  /// The actual LRU list (indices into the cache ordered least to most recently used).
  vtkExodusIICacheLRU LRU;
  //ETX

private:
  vtkExodusIICache( const vtkExodusIICache& ); // Not implemented
  void operator = ( const vtkExodusIICache& ); // Not implemented
};
#endif // __vtkExodusIICache_h
