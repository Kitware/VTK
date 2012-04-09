!!ARBfp1.0

# This is the fragment program for two
# component dependent data with shading

# We need some temporary variables
TEMP index1, index2, normal, finalColor;
TEMP temp1, temp2, temp3;
TEMP sampleColor, sampleOpacity;
TEMP ndotl, ndoth, ndotv;
TEMP lightInfo, lightResult;

# We are going to use the first
# texture coordinate
ATTRIB tex0 = fragment.texcoord[0];

# This is the lighting information
PARAM lightDirection = program.local[0];
PARAM halfwayVector  = program.local[1];
PARAM coefficient    = program.local[2];
PARAM lightDiffColor = program.local[3];
PARAM lightSpecColor = program.local[4];
PARAM viewVector     = program.local[5];
PARAM constants      = program.local[6];

# This is our output color
OUTPUT out = result.color;

# Look up the scalar values / gradient
# magnitude in the first volume
TEX temp1, tex0, texture[0], 3D;

# Look up the gradient direction
# in the third volume
TEX temp2, tex0, texture[2], 3D;

# This normal is stored 0 to 1, change to -1 to 1
# by multiplying by 2.0 then adding -1.0.
MAD normal, temp2, constants.x, constants.y;

# Swizzle this to use (a,r) as texture
# coordinates for color, and (g,b) for
# opacity
SWZ index1, temp1, a, r, 1, 1;
SWZ index2, temp1, g, b, 1, 1;

# Use this coordinate to look up a
# final color in the second texture
# (this is a 2D texture) and the final
# opacity in the fourth texture.
TEX sampleColor, index1, texture[1], 2D;
TEX sampleOpacity, index2, texture[3], 2D;

# Take the dot product of the light
# direction and the normal
DP3 ndotl, normal, lightDirection;

# Take the dot product of the halfway
# vector and the normal
DP3 ndoth, normal, halfwayVector;

DP3 ndotv, normal, viewVector;

# flip if necessary for two sided lighting
MUL temp3, ndotl, constants.y;
CMP ndotl, ndotv, ndotl, temp3;
MUL temp3, ndoth, constants.y;
CMP ndoth, ndotv, ndoth, temp3;

# put the pieces together for a LIT operation
MOV lightInfo.x, ndotl.x;
MOV lightInfo.y, ndoth.x;
MOV lightInfo.w, coefficient.w;

# compute the lighting
LIT lightResult, lightInfo;

# This is the ambient contribution
MUL finalColor, coefficient.x, sampleColor;

# This is the diffuse contribution
MUL temp3, lightDiffColor, sampleColor;
MUL temp3, temp3, lightResult.y;
ADD finalColor, finalColor, temp3;

# This is th specular contribution
MUL temp3, lightSpecColor, lightResult.z;

# Add specular into result so far, and replace
# with the original alpha.
ADD out, finalColor, temp3;
MOV out.w, sampleOpacity.w;

END
