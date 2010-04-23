#include "vtkExodusIICache.h"

#include "vtkDataArray.h"
#include "vtkObjectFactory.h"

// Define VTK_EXO_DBG_CACHE to print cache adds, drops, and replacements.
//#undef VTK_EXO_DBG_CACHE

#define VTK_EXO_PRT_KEY( ckey ) \
  "(" << ckey.Time << ", " << ckey.ObjectType << ", " << ckey.ObjectId << ", " << ckey.ArrayId << ")"
#define VTK_EXO_PRT_ARR( cval ) \
  " [" << cval << "," <<  (cval ? cval->GetActualMemorySize() / 1024. : 0.) << "/" << this->Size << "/" << this->Capacity << "]"
#define VTK_EXO_PRT_ARR2( cval ) \
  " [" << cval << ", " <<  (cval ? cval->GetActualMemorySize() / 1024. : 0.) << "]"

#if 0
static void printCache( vtkExodusIICacheSet& cache, vtkExodusIICacheLRU& lru )
{
  cout << "Cache\n";
  vtkExodusIICacheRef cit;
  for ( cit = cache.begin(); cit != cache.end(); ++cit )
    {
    cout << VTK_EXO_PRT_KEY( cit->first ) << VTK_EXO_PRT_ARR2( cit->second->GetValue() ) << "\n";
    }
  cout << "LRU\n";
  vtkExodusIICacheLRURef lit;
  for ( lit = lru.begin(); lit != lru.end(); ++lit )
    {
    cout << VTK_EXO_PRT_KEY( (*lit)->first ) << "\n";
    }
}
#endif // 0

// ============================================================================
vtkExodusIICacheEntry::vtkExodusIICacheEntry()
{
  this->Value = 0;
}

vtkExodusIICacheEntry::vtkExodusIICacheEntry( vtkDataArray* arr )
{
  this->Value = arr;
  if ( arr )
    this->Value->Register( 0 );
}
vtkExodusIICacheEntry::~vtkExodusIICacheEntry()
{
  if ( this->Value )
    this->Value->Delete();
}

vtkExodusIICacheEntry::vtkExodusIICacheEntry( const vtkExodusIICacheEntry& other )
{
  this->Value = other.Value;
  if ( this->Value )
    this->Value->Register( 0 );
}

#if 0
void printLRUBack( vtkExodusIICacheRef& cit )
{
  cout << "Key is " << VTK_EXO_PRT_KEY( cit->first ) << "\n";
}
#endif // 0

// ============================================================================

vtkStandardNewMacro(vtkExodusIICache);

vtkExodusIICache::vtkExodusIICache()
{
  this->Size = 0.;
  this->Capacity = 2.;
}

vtkExodusIICache::~vtkExodusIICache()
{
  this->ReduceToSize( 0. );
}

void vtkExodusIICache::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "Capacity: " << this->Capacity << " MiB\n";
  os << indent << "Size: " << this->Size << " MiB\n";
  os << indent << "Cache: " << &this->Cache << " (" << this->Cache.size() << ")\n";
  os << indent << "LRU: " << &this->LRU << "\n";
}

void vtkExodusIICache::Clear()
{
  //printCache( this->Cache, this->LRU );
  this->ReduceToSize( 0. );
}

void vtkExodusIICache::SetCacheCapacity( double sizeInMiB )
{
  if ( sizeInMiB == this->Capacity )
    return;

  if ( this->Size > sizeInMiB )
    {
    this->ReduceToSize( sizeInMiB );
    }

  this->Capacity =  sizeInMiB < 0 ? 0 : sizeInMiB;
}

int vtkExodusIICache::ReduceToSize( double newSize )
{
  int deletedSomething = 0;
  while ( this->Size > newSize && ! this->LRU.empty() )
    {
    vtkExodusIICacheRef cit( this->LRU.back() );
    vtkDataArray* arr = cit->second->Value;
    if ( arr )
      {
      deletedSomething = 1;
      double arrSz = (double) arr->GetActualMemorySize() / 1024.;
      this->Size -= arrSz;
#ifdef VTK_EXO_DBG_CACHE
      cout << "Dropping " << VTK_EXO_PRT_KEY( cit->first ) << VTK_EXO_PRT_ARR( arr ) << "\n";
#endif // VTK_EXO_DBG_CACHE
      if ( this->Size <= 0 )
        {
        if ( this->Cache.size() == 0 )
          this->Size = 0.;
        else
          this->RecomputeSize(); // oops, FP roundoff
        }
      }
    else
      {
#ifdef VTK_EXO_DBG_CACHE
      cout << "Dropping " << VTK_EXO_PRT_KEY( cit->first ) << VTK_EXO_PRT_ARR( arr ) << "\n";
#endif // VTK_EXO_DBG_CACHE
      }

    delete cit->second;
    this->Cache.erase( cit );
    this->LRU.pop_back();
    }

  if ( this->Cache.size() == 0 )
    {
    this->Size = 0;
    }

  return deletedSomething;
}

void vtkExodusIICache::Insert( vtkExodusIICacheKey& key, vtkDataArray* value )
{
  double vsize = value ? value->GetActualMemorySize() / 1024. : 0.;

  vtkExodusIICacheRef it = this->Cache.find( key );
  if ( it != this->Cache.end() )
    {
    if ( it->second->Value == value )
      return;

    // Remove existing array and put in our new one.
    this->Size -= vsize;
    if ( this->Size <= 0 )
      {
      this->RecomputeSize();
      }
    this->ReduceToSize( this->Capacity - vsize );
    it->second->Value->Delete();
    it->second->Value = value;
    it->second->Value->Register( 0 ); // Since we re-use the cache entry, the constructor's Register won't get called.
    this->Size += vsize;
#ifdef VTK_EXO_DBG_CACHE
    cout << "Replacing " << VTK_EXO_PRT_KEY( it->first ) << VTK_EXO_PRT_ARR( value ) << "\n";
#endif // VTK_EXO_DBG_CACHE
    this->LRU.erase( it->second->LRUEntry );
    it->second->LRUEntry = this->LRU.insert( this->LRU.begin(), it );
    }
  else
    {
    this->ReduceToSize( this->Capacity - vsize );
    vtkstd::pair<const vtkExodusIICacheKey,vtkExodusIICacheEntry*> entry( key, new vtkExodusIICacheEntry(value) );
    vtkstd::pair<vtkExodusIICacheSet::iterator, bool> iret = this->Cache.insert( entry );
    this->Size += vsize;
#ifdef VTK_EXO_DBG_CACHE
    cout << "Adding " << VTK_EXO_PRT_KEY( key ) << VTK_EXO_PRT_ARR( value ) << "\n";
#endif // VTK_EXO_DBG_CACHE
    iret.first->second->LRUEntry = this->LRU.insert( this->LRU.begin(), iret.first );
    }
  //printCache( this->Cache, this->LRU );
}

vtkDataArray*& vtkExodusIICache::Find( vtkExodusIICacheKey key )
{
  static vtkDataArray* dummy = 0;

  vtkExodusIICacheRef it = this->Cache.find( key );
  if ( it != this->Cache.end() )
    {
    this->LRU.erase( it->second->LRUEntry );
    it->second->LRUEntry = this->LRU.insert( this->LRU.begin(), it );
    return it->second->Value;
    }

  dummy = 0;
  return dummy;
}

int vtkExodusIICache::Invalidate( vtkExodusIICacheKey key )
{
  vtkExodusIICacheRef it = this->Cache.find( key );
  if ( it != this->Cache.end() )
    {
#ifdef VTK_EXO_DBG_CACHE
    cout << "Dropping " << VTK_EXO_PRT_KEY( it->first ) << VTK_EXO_PRT_ARR( it->second->Value ) << "\n";
#endif // VTK_EXO_DBG_CACHE
    this->LRU.erase( it->second->LRUEntry );
    if ( it->second->Value )
      {
      this->Size -= it->second->Value->GetActualMemorySize() / 1024.;
      }
    delete it->second;
    this->Cache.erase( it );

    if ( this->Size <= 0 )
      {
      if ( this->Cache.size() == 0 )
        this->Size = 0.;
      else
        this->RecomputeSize(); // oops, FP roundoff
      }

    return 1;
    }
  return 0;
}

int vtkExodusIICache::Invalidate( vtkExodusIICacheKey key, vtkExodusIICacheKey pattern )
{
  vtkExodusIICacheRef it;
  int nDropped = 0;
  it = this->Cache.begin();
  while ( it != this->Cache.end() )
    {
    if ( ! it->first.match( key, pattern ) )
      {
      ++it;
      continue;
      }

#ifdef VTK_EXO_DBG_CACHE
    cout << "Dropping " << VTK_EXO_PRT_KEY( it->first ) << VTK_EXO_PRT_ARR( it->second->Value ) << "\n";
#endif // VTK_EXO_DBG_CACHE
    this->LRU.erase( it->second->LRUEntry );
    if ( it->second->Value )
      {
      this->Size -= it->second->Value->GetActualMemorySize() / 1024.;
      }
    vtkExodusIICacheRef tmpIt = it++;
    delete tmpIt->second;
    this->Cache.erase( tmpIt );

    if ( this->Size <= 0 )
      {
      if ( this->Cache.size() == 0 )
        this->Size = 0.;
      else
        this->RecomputeSize(); // oops, FP roundoff
      }

    ++nDropped;
    }
  return nDropped;
}

void vtkExodusIICache::RecomputeSize()
{
  this->Size = 0.;
  vtkExodusIICacheRef it;
  for ( it = this->Cache.begin(); it != this->Cache.end(); ++it )
    {
    if ( it->second->Value )
      {
      this->Size += (double)it->second->Value->GetActualMemorySize() / 1024.;
      }
    }
}
