#ifndef    __FTCallbackVector__
#define    __FTCallbackVector__

/*
  Provides a non-STL alternative to the STL vector<...>
  used inside FTLibrary.
*/

#ifdef FT_VECTOR_CLASS_NAME
#undef FT_VECTOR_CLASS_NAME
#endif
#define FT_VECTOR_CLASS_NAME FTCallbackVector

#ifdef FT_VECTOR_ITEM_TYPE
#undef FT_VECTOR_ITEM_TYPE
#endif
#define FT_VECTOR_ITEM_TYPE FTCallback

#include "FTVector.h"

#endif  //  __FTCallbackVector__
