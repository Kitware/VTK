#include "vtkExodusIICache.h"

#include "vtkDataArray.h"
#include "vtkObjectFactory.h"

#define VTK_EXO_PRT_KEY( ckey ) \
  "(" << ckey.Time << ", " << ckey.ObjectType << ", " << ckey.ObjectId << ", " << ckey.ArrayId << ")"
#define VTK_EXO_PRT_ARR( cval ) \
  " [" << cval << "," <<  (cval ? cval->GetActualMemorySize() / 1024. : 0.) << "/" << this->Size << "/" << this->Capacity << "]"
#define VTK_EXO_PRT_ARR2( cval ) \
  " [" << cval << ", " <<  (cval ? cval->GetActualMemorySize() / 1024. : 0.) << "]"

#if 0
static void printCache( vtkExodusIICacheEntry::CacheSet& cache, vtkExodusIICacheEntry::CacheLRU& lru )
{
  cout << "Cache\n";
  vtkExodusIICacheEntry::CacheRef cit;
  for ( cit = cache.begin(); cit != cache.end(); ++cit )
    {
    cout << VTK_EXO_PRT_KEY( cit->first ) << VTK_EXO_PRT_ARR2( cit->second.GetValue() ) << "\n";
    }
  cout << "LRU\n";
  vtkExodusIICacheEntry::LRURef lit;
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
void printLRUBack( vtkExodusIICacheEntry::CacheRef& cit )
{
  cout << "Key is " << VTK_EXO_PRT_KEY( cit->first ) << "\n";
}
#endif // 0

// ============================================================================

vtkCxxRevisionMacro(vtkExodusIICache,"1.2");
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
    vtkExodusIICacheEntry::CacheRef cit( this->LRU.back() );
    vtkDataArray* arr = cit->second.Value;
    if ( arr )
      {
      deletedSomething = 1;
      double arrSz = (double) arr->GetActualMemorySize() / 1024.;
      this->Size -= arrSz;
      cout << "Dropping " << VTK_EXO_PRT_KEY( cit->first ) << VTK_EXO_PRT_ARR( arr ) << "\n";
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
      cout << "Dropping " << VTK_EXO_PRT_KEY( cit->first ) << VTK_EXO_PRT_ARR( arr ) << "\n";
      }

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

  vtkExodusIICacheEntry::CacheRef it = this->Cache.find( key );
  if ( it != this->Cache.end() )
    {
    if ( it->second.Value == value )
      return;

    // Remove existing array and put in our new one.
    this->Size -= vsize;
    if ( this->Size <= 0 )
      {
      this->RecomputeSize();
      }
    this->ReduceToSize( this->Capacity - vsize );
    it->second.Value->Delete();
    it->second.Value = value;
    this->Size += vsize;
    cout << "Replacing " << VTK_EXO_PRT_KEY( it->first ) << VTK_EXO_PRT_ARR( value ) << "\n";
    this->LRU.erase( it->second.LRUEntry );
    it->second.LRUEntry = this->LRU.insert( this->LRU.begin(), it );
    }
  else
    {
    this->ReduceToSize( this->Capacity - vsize );
    vtkstd::pair<vtkExodusIICacheEntry::CacheRef, bool> iret = this->Cache.insert(
      vtkstd::pair<vtkExodusIICacheKey,vtkExodusIICacheEntry>( key, vtkExodusIICacheEntry(value) ) );
    this->Size += vsize;
    cout << "Adding " << VTK_EXO_PRT_KEY( key ) << VTK_EXO_PRT_ARR( value ) << "\n";
    iret.first->second.LRUEntry = this->LRU.insert( this->LRU.begin(), iret.first );
    }
  //printCache( this->Cache, this->LRU );
}

vtkDataArray*& vtkExodusIICache::Find( vtkExodusIICacheKey key )
{
  static vtkDataArray* dummy = 0;

  vtkExodusIICacheEntry::CacheRef it = this->Cache.find( key );
  if ( it != this->Cache.end() )
    {
    this->LRU.erase( it->second.LRUEntry );
    it->second.LRUEntry = this->LRU.insert( this->LRU.begin(), it );
    return it->second.Value;
    }

  dummy = 0;
  return dummy;
}

int vtkExodusIICache::Invalidate( vtkExodusIICacheKey key )
{
  vtkExodusIICacheEntry::CacheRef it = this->Cache.find( key );
  if ( it != this->Cache.end() )
    {
    cout << "Dropping " << VTK_EXO_PRT_KEY( it->first ) << VTK_EXO_PRT_ARR( it->second.Value ) << "\n";
    this->LRU.erase( it->second.LRUEntry );
    if ( it->second.Value )
      {
      this->Size -= it->second.Value->GetActualMemorySize() / 1024.;
      }
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
  vtkExodusIICacheEntry::CacheRef it;
  int nDropped = 0;
  it = this->Cache.begin();
  while ( it != this->Cache.end() )
    {
    if ( ! it->first.match( key, pattern ) )
      {
      ++it;
      continue;
      }

    cout << "Dropping " << VTK_EXO_PRT_KEY( it->first ) << VTK_EXO_PRT_ARR( it->second.Value ) << "\n";
    this->LRU.erase( it->second.LRUEntry );
    if ( it->second.Value )
      {
      this->Size -= it->second.Value->GetActualMemorySize() / 1024.;
      }
    vtkExodusIICacheEntry::CacheRef tmpIt = it++;
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
  vtkExodusIICacheEntry::CacheRef it;
  for ( it = this->Cache.begin(); it != this->Cache.end(); ++it )
    {
    if ( it->second.Value )
      {
      this->Size += (double)it->second.Value->GetActualMemorySize() / 1024.;
      }
    }
}
