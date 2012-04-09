!!ARBfp1.0
	
# This is the fragment program for two
# component dependent data with no shading	

# We need some temporary variables		
TEMP temp1, temp2, temp3;
TEMP finalColor, finalOpacity;
	 
# We are going to use the first 
# texture coordinate		
ATTRIB tex0 = fragment.texcoord[0];
	
# This is our output color
OUTPUT out = result.color;

# Look up the scalar values / gradient
# magnitude in the first volume	
TEX temp1, tex0, texture[0], 3D;

# Swizzle this to use (a,r) as texture
# coordinates for color, and (g,b) for
# opacity		
SWZ temp2, temp1, a, r, 1, 1;
SWZ temp3, temp1, g, b, 1, 1;

# Use the (a,r) coordinate to look up a 
# final color in the second texture
# (this is a 2D texture)			
TEX finalColor, temp2, texture[1], 2D;

# Use the (g,b)  coordinate to look up a 
# final opacity in the fourth texture
# (this is a 2D texture)			
TEX finalOpacity, temp3, texture[3], 2D;
	
# Combine these into the result
MOV out, finalColor;
MOV out.w, finalOpacity.w; 
END





	