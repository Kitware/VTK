//
//BTX - begin tcl exclude
#define CELLTRIANGLES(CELLPTIDS, TYPE, IDX, PTID0, PTID1, PTID2) \
	{ switch( TYPE ) \
	  { \
	  case VTK_TRIANGLE: \
	  case VTK_POLYGON: \
	  case VTK_QUAD: \
	    PTID0 = CELLPTIDS[0]; \
	    PTID1 = CELLPTIDS[(IDX)+1]; \
	    PTID2 = CELLPTIDS[(IDX)+2]; \
	    break; \
	  case VTK_TRIANGLE_STRIP: \
	    PTID0 = CELLPTIDS[IDX]; \
	    PTID1 = CELLPTIDS[(IDX)+1+((IDX)&1)]; \
	    PTID2 = CELLPTIDS[(IDX)+2-((IDX)&1)]; \
	    break; \
	  default: \
	    PTID0 = PTID1 = PTID2 = -1; \
	  } }
//ETX - end tcl exclude
//
