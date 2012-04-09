!!ARBfp1.0
#------------------------------------------------------------------------------
# Copyright 2005 by University of Utah
#
# Hardware-Assisted Visibility Sorting
#
# The program consists of the following steps:
#
# 1. Clear k-buffers entries 1 and 2 to -1.0
# 
# The following textures are used:
# 
#   Tex 1: k-buffer entries 1 and 2 (same)
#

# -----------------------------------------------------------------------------
# use the ATI_draw_buffers extension
OPTION ATI_draw_buffers;
# this does not matter now, but will matter on future hardware
OPTION ARB_precision_hint_nicest;

MOV result.color[0], 0.0;
MOV result.color[1], -1.0;

END
