#ifndef    __FTPointVector__
#define    __FTPointVector__

/*
  Provides a non-STL alternative to the STL vector<...>
  used inside FTVectoriser.
*/

#ifdef FT_VECTOR_CLASS_NAME
#undef FT_VECTOR_CLASS_NAME
#endif
#define FT_VECTOR_CLASS_NAME FTPointVector

#ifdef FT_VECTOR_ITEM_TYPE
#undef FT_VECTOR_ITEM_TYPE
#endif
#define FT_VECTOR_ITEM_TYPE ftPoint

#include "FTVector.h"

#endif  //  __FTPointVector__
