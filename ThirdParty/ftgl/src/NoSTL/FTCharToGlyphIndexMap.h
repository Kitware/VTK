#ifndef    __FTCharToGlyphIndexMap__
#define    __FTCharToGlyphIndexMap__

/*
  Provides a non-STL alternative to the STL map<unsigned long, unsigned long>
  which maps character codes to glyph indices inside FTCharmap.
  
  Implementation:
    - NumberOfBuckets buckets are considered.
    - Each bucket has BucketSize entries.
    - When the glyph index for the character code C has to be stored, the 
      bucket this character belongs to is found using 'C div BucketSize'. 
      If this bucket has not been allocated yet, do it now.
      The entry in the bucked is found using 'C mod BucketSize'. 
      If it is set to IndexNotFound, then the glyph entry has not been set.
    - Try to mimic the calls made to the STL map API.

  Caveats:
    - The glyph index is now a signed long instead of unsigned long, so
      the special value IndexNotFound (= -1) can be used to specify that the 
      glyph index has not been stored yet.
*/

#include "FTGL.h"

#ifdef FTGL_USE_NAMESPACE
namespace ftgl
{
#endif

class FTGL_EXPORT FTCharToGlyphIndexMap
{
public:
  
  typedef unsigned long CharacterCode;
  typedef signed long GlyphIndex;

  enum 
  {
    NumberOfBuckets = 256,
    BucketSize = 256,
    IndexNotFound = -1
  };

  FTCharToGlyphIndexMap();

  virtual ~FTCharToGlyphIndexMap();
  
  void clear();

  const GlyphIndex* find(CharacterCode c);

  void insert(CharacterCode c, GlyphIndex g);
  
private:

  GlyphIndex** Indices;
};


inline FTCharToGlyphIndexMap::FTCharToGlyphIndexMap()
{
  this->Indices = 0;
}


// Free all buckets

inline void FTCharToGlyphIndexMap::clear()
{
  if (this->Indices)
    {
    for (int i = 0; i < FTCharToGlyphIndexMap::NumberOfBuckets; i++)
      {
      if (this->Indices[i])
        {
        delete [] this->Indices[i];
        this->Indices[i] = 0;
        }
      }
    }
}

inline FTCharToGlyphIndexMap::~FTCharToGlyphIndexMap()
{
  if (this->Indices)
    {
    // Free all buckets

    this->clear();

    // Free main structure

    delete [] this->Indices;
    this->Indices = 0;
    }
}

// Find index corresponding to char code

inline const FTCharToGlyphIndexMap::GlyphIndex* FTCharToGlyphIndexMap::find(FTCharToGlyphIndexMap::CharacterCode c)
{
  if (!this->Indices)
    {
    return 0;
    }

  // Find position of char code in buckets

  div_t pos = div(c, FTCharToGlyphIndexMap::BucketSize);

  if (!this->Indices[pos.quot])
    {
    return 0;
    }

  const FTCharToGlyphIndexMap::GlyphIndex *ptr = &this->Indices[pos.quot][pos.rem];
  if (*ptr == FTCharToGlyphIndexMap::IndexNotFound)
    {
    return 0;
    }

  return ptr;
}

// Insert index corresponding to char code

inline void FTCharToGlyphIndexMap::insert(FTCharToGlyphIndexMap::CharacterCode c,
                                      FTCharToGlyphIndexMap::GlyphIndex g)
{
  if (!this->Indices)
    {
    this->Indices = new GlyphIndex* [FTCharToGlyphIndexMap::NumberOfBuckets];
    for (int i = 0; i < FTCharToGlyphIndexMap::NumberOfBuckets; i++)
      {
        this->Indices[i] = 0;
      }
    }

  // Find position of char code in buckets

  div_t pos = div(c, FTCharToGlyphIndexMap::BucketSize);

  // Allocate bucket if does not exist yet

  if (!this->Indices[pos.quot])
    {
    this->Indices[pos.quot] = new GlyphIndex [FTCharToGlyphIndexMap::BucketSize];
    for (int i = 0; i < FTCharToGlyphIndexMap::BucketSize; i++)
      {
      this->Indices[pos.quot][i] = FTCharToGlyphIndexMap::IndexNotFound;
      }
    }
  
  this->Indices[pos.quot][pos.rem] = g;
}

#ifdef FTGL_USE_NAMESPACE
} // namespace ftgl
#endif

#endif  //  __FTCharToGlyphIndexMap__
