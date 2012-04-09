!!ARBfp1.0

# This is the fragment program for four
# component dependent data with no shading

# We need some temporary variables
TEMP temp1, temp2;
TEMP finalColor, finalOpacity;

# We are going to use the first
# texture coordinate
ATTRIB tex0 = fragment.texcoord[0];

# This is our output color
OUTPUT out = result.color;

# Look up the color in the first volume
TEX finalColor, tex0, texture[0], 3D;

# Look up the fourth scalar / gradient
# magnitude in the second volume
TEX temp1, tex0, texture[1], 3D;

# Swizzle this to use (a,r) as texture
# coordinates for color, and (g,b) for
# opacity
SWZ temp2, temp1, a, r, 1, 1;

# Use the (a,r) coordinate to look up a
# final opacity in the fourth texture
# (this is a 2D texture)
TEX finalOpacity, temp2, texture[3], 2D;

# Combine these into the result
MOV out, finalColor;
MOV out.w, finalOpacity.w;
END
