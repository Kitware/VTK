#ifndef    __FTContourVector__
#define    __FTContourVector__

/*
  Provides a non-STL alternative to the STL vector<...>
  used inside FTVectoriser.
*/

#ifdef FT_VECTOR_CLASS_NAME
#undef FT_VECTOR_CLASS_NAME
#endif
#define FT_VECTOR_CLASS_NAME FTContourVector

#ifdef FT_VECTOR_ITEM_TYPE
#undef FT_VECTOR_ITEM_TYPE
#endif
#define FT_VECTOR_ITEM_TYPE FTContour*

#include "FTVector.h"

#endif  //  __FTContourVector__
