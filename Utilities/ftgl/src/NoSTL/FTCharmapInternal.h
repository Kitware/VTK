#ifndef    __FTCharmapInternal__
#define    __FTCharmapInternal__

/*
  Provides a non-STL alternative to the map<unsigned long, unsigned long>
  which maps character codes to glyph indices inside FTCharmap.
  
  Implementation:
    - NumberOfBuckets buckets are considered.
    - Each bucket has BucketSize entries.
    - When the glyph index for the character code C has to be stored, the 
      bucket this character belongs to is found using 'C div BucketSize'. 
      If this bucket has not been allocated yet, do it now.
      The entry in the bucked is found using 'C mod BucketSize'. 
      If it is set to IndexNotFound, then the glyph entry has not been set.
    - Try to mimic the calls made to the map API.

  Caveats:
    - The special value IndexNotFound will be used to specify that the 
      glyph index has not been stored yet.
*/

#include "FTGL.h"

class FTGL_EXPORT FTCharmapInternal
{
public:
  
  typedef unsigned long CharacterCode;
  typedef unsigned long GlyphIndex;

  static const int NumberOfBuckets;
  static const int BucketSize;
  static const GlyphIndex IndexNotFound;

  FTCharmapInternal();

  virtual ~FTCharmapInternal();
  
  
  void clear();

  const GlyphIndex* find(CharacterCode c);

  void insert(CharacterCode c, GlyphIndex g);
  
private:

  GlyphIndex** Indices;
};


#endif  //  __FTCharmapInternal__
