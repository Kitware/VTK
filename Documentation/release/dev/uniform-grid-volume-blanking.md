# Uniform grid volume blanking

The GPU based ray-cast volume mapper now supports direct volume rendering with blanking of uniform
grids. This means that if the input provided to the volume mapper is a uniform grid and if it has
ghost cells/points, the respective cells will be blanked.

![Uniform-Grid-Blanking](TestGPURayCastVolumeUniformGridBlanking.png)
