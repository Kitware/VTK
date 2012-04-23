!!ARBvp1.0
# -----------------------------------------------------------------------------
# Copyright 2005 by University of Utah
#
# This program fixes perspective-correct texture lookups
# -----------------------------------------------------------------------------

ATTRIB iPos = vertex.position;
ATTRIB iTex0 = vertex.texcoord[0];
PARAM mvp[4] = { state.matrix.mvp };
PARAM mv[4] = { state.matrix.modelview };
OUTPUT oPos = result.position;
OUTPUT oTex0 = result.texcoord[0];
OUTPUT oTex1 = result.texcoord[1];

# -----------------------------------------------------------------------------
# transform vertex to clip coordinates
DP4 oPos.x, mvp[0], iPos;
DP4 oPos.y, mvp[1], iPos;
DP4 oPos.z, mvp[2], iPos;
DP4 oPos.w, mvp[3], iPos;

# -----------------------------------------------------------------------------
# transform vertex to eye coordinates
DP4 oTex1.x, mv[0], iPos;
DP4 oTex1.y, mv[1], iPos;
DP4 oTex1.z, mv[2], iPos;

# -----------------------------------------------------------------------------
# texcoord 0 contains the scalar data value
MOV oTex0, iTex0;

END
