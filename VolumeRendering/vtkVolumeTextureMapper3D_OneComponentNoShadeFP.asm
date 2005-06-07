!!ARBfp1.0
	
# This is the fragment program for one
# component data with no shading	

# We need some temporary variables		
TEMP temp1, temp2, temp3;

# We are going to use the first 
# texture coordinate		
ATTRIB tex0 = fragment.texcoord[0];
	
# This is our output color
OUTPUT out = result.color;

# Look up the scalar value / gradient
# magnitude in the first volume	
TEX temp1, tex0, texture[0], 3D;

# Swizzle this to use (a,r) as texture
# coordinates	
SWZ temp2, temp1, a, r, 1, 1;

# Use this coordinate to look up a 
# final color in the second texture
# (this is a 2D texture)			
TEX temp3, temp2, texture[1], 2D;

# That's our result and we are done
MOV out, temp3;
END





	