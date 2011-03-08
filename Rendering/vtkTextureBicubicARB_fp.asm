!!ARBfp1.0

# bicubic texture interpolator
# author: David Gobbi

# 1 parameter (max 24)
# 1 texture (max 2)
# 10 temporaries (max 12)
# 16 texture lookups (max 24)
# 55 instructions (max 72)

# width, height, xspacing, yspacing
PARAM texdim = program.local[0];
TEMP coord, coord2;
TEMP weightx, weighty;
TEMP t1, t2, t3, t4;
TEMP c, c1;

# compute the {rx, ry, fx, fy} fraction vector
MAD coord, fragment.texcoord.xyxy, texdim.xyxy, {0.5, 0.5, 0.5, 0.5};
FRC coord, coord;
SUB coord.xy, {1, 1, 1, 1}, coord;

# compute the x weights
MAD weightx, coord.zzxx, {0.5, 1.5, 1.5, 0.5}, {0,-1,-1, 0};
MAD weightx, weightx, coord.xzxz, {0,-1,-1, 0};
MUL weightx, weightx, -coord.xxzz;

# compute the y weights
MAD weighty, coord.wwyy, {0.5, 1.5, 1.5, 0.5}, {0,-1,-1, 0};
MAD weighty, weighty, coord.ywyw, {0,-1,-1, 0};
MUL weighty, weighty, -coord.yyww;

# get the texture coords for the coefficients
ADD coord, coord.xyxy, {-2,-2,-1,-2};
MAD coord, coord, texdim.zwzw, fragment.texcoord.xyxy;
MAD coord2, texdim.zwzw, {2, 0, 2, 0}, coord;

# do first set of X lookups
TEX t1, coord.xyzw, texture, 2D;
TEX t2, coord.zwxy, texture, 2D;
TEX t3, coord2.xyzw, texture, 2D;
TEX t4, coord2.zwxy, texture, 2D;

# multiply by the weights
MUL c1, t1, weightx.xxxx;
MAD c1, t2, weightx.yyyy, c1;
MAD c1, t3, weightx.zzzz, c1;
MAD c1, t4, weightx.wwww, c1;
MUL c, weighty.xxxx, c1;

# advance to next row
ADD coord.yw, coord, texdim.wwww;
ADD coord2.yw, coord2, texdim.wwww;

# do second set of X lookups
TEX t1, coord.xyzw, texture, 2D;
TEX t2, coord.zwxy, texture, 2D;
TEX t3, coord2.xyzw, texture, 2D;
TEX t4, coord2.zwxy, texture, 2D;

# multiply by the weights
MUL c1, t1, weightx.xxxx;
MAD c1, t2, weightx.yyyy, c1;
MAD c1, t3, weightx.zzzz, c1;
MAD c1, t4, weightx.wwww, c1;
MAD c, weighty.yyyy, c1, c;

# advance to next row
ADD coord.yw, coord, texdim.wwww;
ADD coord2.yw, coord2, texdim.wwww;

# do third set of X lookups
TEX t1, coord.xyzw, texture, 2D;
TEX t2, coord.zwxy, texture, 2D;
TEX t3, coord2.xyzw, texture, 2D;
TEX t4, coord2.zwxy, texture, 2D;

# multiply by the weights
MUL c1, t1, weightx.xxxx;
MAD c1, t2, weightx.yyyy, c1;
MAD c1, t3, weightx.zzzz, c1;
MAD c1, t4, weightx.wwww, c1;
MAD c, weighty.zzzz, c1, c;

# advance to next row
ADD coord.yw, coord, texdim.wwww;
ADD coord2.yw, coord2, texdim.wwww;

# do fourth set of X lookups
TEX t1, coord.xyzw, texture, 2D;
TEX t2, coord.zwxy, texture, 2D;
TEX t3, coord2.xyzw, texture, 2D;
TEX t4, coord2.zwxy, texture, 2D;

# multiply by the weights
MUL c1, t1, weightx.xxxx;
MAD c1, t2, weightx.yyyy, c1;
MAD c1, t3, weightx.zzzz, c1;
MAD c1, t4, weightx.wwww, c1;
MAD c, weighty.wwww, c1, c;

# output the color
MUL result.color, fragment.color, c;

END
