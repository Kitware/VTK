#ifndef    __FTGlyphVector__
#define    __FTGlyphVector__

/*
  Provides a non-STL alternative to the STL vector<...>
  used inside FTGlyphContainer.
*/

#ifdef FT_VECTOR_CLASS_NAME
#undef FT_VECTOR_CLASS_NAME
#endif
#define FT_VECTOR_CLASS_NAME FTGlyphVector

#ifdef FT_VECTOR_ITEM_TYPE
#undef FT_VECTOR_ITEM_TYPE
#endif
#define FT_VECTOR_ITEM_TYPE FTGlyph*

#include "FTVector.h"

#endif  //  __FTGlyphVector__
