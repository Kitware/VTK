#if defined(_MSC_VER)

#if _MSC_VER <= 1600
#define _Out_writes_( x )
#endif

#if _MSC_VER <= 1500
#pragma include_alias( <stdint.h>, <stddef.h> )
typedef __int64 int64_t;
#endif

#endif
