!!ARBfp1.0
# -----------------------------------------------------------------------------
# Copyright 2005 by University of Utah
#
# Hardware-Assisted Visibility Sorting
#	 
# The program consists of the following steps:
#
# 1. Find the first and second entries in the fixed size k-buffer list sorted 
#    by d (2+1 entries)
# 2. Perform a 3D pre-integrated transfer function lookup using front and back
#    scalar data values + the segment length computed from the distance values
#    of the first and second entries from the k-buffer.
# 3. Composite the color and opacity from the transfer funcion with the
#    color and opacity from the framebuffer. Discard winning k-buffer entry,
#    write the remaining k-buffer entries.
#
# The following textures are used:
# 
#   Tex 0: framebuffer (pbuffer, 2D RGBA 16/32 bpp float)
#   Tex 1: k-buffer entries 1 and 2 (same)
#   Tex 4: transfer function (regular, 3D RGBA 8/16 bpp int)
#

# -----------------------------------------------------------------------------
# use the ATI_draw_buffers extension
OPTION ATI_draw_buffers;
# this does not matter now, but will matter on future hardware
OPTION ARB_precision_hint_nicest;

# -----------------------------------------------------------------------------
# input and temporaries
ATTRIB p = fragment.position; # fragment position in screen space
ATTRIB v = fragment.texcoord[0]; # v.x = scalar value in
ATTRIB e = fragment.texcoord[1]; # fragment position in eye space
PARAM sz = program.local[0]; # texture scale and max gap length parameters
                             # {1/pw, 1/ph, max, not_used)}
PARAM half = { 0.5, 0.5, 0.0, 0.0 };
PARAM exp = { 0.0, 0.0, 0.0, 1.44269504 }; # 1/ln2 
		
TEMP a0, a1, a2; # k-buffer entries
TEMP r0, r1, r2, r3; # sorted results
TEMP c, c0; # color and opacity
TEMP t; # temporary (boolean flag for min/max, dependent texture coordinate,
        # pbuffer texture coordinate, fragment to eye distance)
TEMP colorBack, colorFront;
TEMP taud, zeta, gamma, Psi;

# -----------------------------------------------------------------------------
# compute texture coordinates from window position so that it is not 
# interpolated perspective correct.  Then look up the color and opacity from 
# the framebuffer
MUL t, p, sz; # t.xy = p.xy * sz.xy, only x and y are used for texture lookup
TEX c0, t, texture[0], 2D; # framebuffer color

# -----------------------------------------------------------------------------
# Check opacity and kill fragment if it is greater than tolerance
SUB t.w, 0.99, c0.w;
KIL t.w;

# -----------------------------------------------------------------------------
# set up the k-buffer entries a0, a1
# each k-buffer entry contains the scalar data value in x or z
# and the distance value in y or w
TEX a1, t, texture[1], 2D; # k-buffer entry 1
MOV a2, a1.zwzw; # k-buffer entry 2

# -----------------------------------------------------------------------------
# compute fragment to eye distance
DP3 t, e, e;
RSQ t.y, t.y;
MUL a0.y, t.x, t.y; # fragment to eye distance
MOV a0.x, v.x; # scalar data value

# -----------------------------------------------------------------------------
# find fragment with minimum d (r0), save the rest to r1, r2, r3

# r0 = min_d(a0, a1); r1 = max_d(a0, a1);
SUB t.w, a0.y, a1.y; # t.w < 0 iff a0.y < a1.y
CMP r1, t.w, a1, a0; # r1 = (a0.y < a1.y ? a1 : a0)
CMP r0, t.w, a0, a1; # r0 = (a0.y < a1.y ? a0 : a1)

# r0 = min_d(r0, a2); r2 = max_d(r0, a2)
SUB t.w, r0.y, a2.y; # t.w < 0 iff r0.y < a2.y
CMP r2, t.w, a2, r0; # r2 = (r0.y < a2.y ? a2 : r0)
CMP r0, t.w, r0, a2; # r0 = (r0.y < a2.y ? r0 : a2)

# -----------------------------------------------------------------------------
# find fragment with minimum d (r3) from r1, r2

# r3 = min_d(r1, r2);
SUB t.w, r1.y, r2.y; # t.w < 0 iff r1.y < r2.y
CMP r3, t.w, r1, r2; # r3 = (r1.y < r2.y ? r1 : r2);

# -----------------------------------------------------------------------------
# set up texture coordinates for transfer function lookup
MOV t.x, r0.x; # front scalar
MOV t.y, r3.x; # back scalar
SUB t.z, r3.y, r0.y; # distance between front and back fragment

# -----------------------------------------------------------------------------
# nullify fragment if distance is greater than unit scale (non-convexities)
SUB t.w, sz.z, t.z; 
CMP t.z, t.w, 0.0, t.z; 

# -----------------------------------------------------------------------------
# transfer function lookup
TEX colorFront, t.x, texture[3], 1D;
TEX colorBack,  t.y, texture[3], 1D;
MUL taud.x, t.z, colorBack.a;
MUL taud.y, t.z, colorFront.a;
	
# -----------------------------------------------------------------------------
# compute zeta = exp(-0.5*(taudf+taudb))
DP3 zeta.w, taud, half;
MUL zeta.w, exp.w, zeta.w;
EX2 zeta.w, -zeta.w;
	
# -----------------------------------------------------------------------------
# compute gamma = taud/(1+taud);
ADD t, taud, 1.0;
RCP t.x, t.x;
RCP t.y, t.y; 
MUL gamma, taud, t; 

# -----------------------------------------------------------------------------
# lookup Psi
TEX Psi.w, gamma, texture[2], 2D;

# -----------------------------------------------------------------------------
# compute color = cb(psi-zeta) + cf(1.0-psi) 
SUB t.w, Psi.w, zeta.w;
MUL colorBack, colorBack, t.w; 
SUB t.w, 1.0, Psi.w;	
MUL colorFront, colorFront, t.w; 
ADD c, colorBack, colorFront; 
SUB c.a, 1.0, zeta.w;
	
# -----------------------------------------------------------------------------
# nullify winning entry if the scalar value < 0
CMP c, r0.x, 0.0, c;

# -----------------------------------------------------------------------------
# composite color with the color from the framebuffer !!!front to back!!! 
SUB t.w, 1.0, c0.w;
MAD result.color[0], c, t.w, c0;

# -----------------------------------------------------------------------------
# write remaining k-buffer entries
MOV r1.zw, r2.xxxy;
MOV result.color[1], r1;
END
