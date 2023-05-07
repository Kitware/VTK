## Improve robustness of wireframe rendering

The shading code for wireframe rendering has been improved.  Near-vertical
lines used to be shaded black in wireframe mode on some GPUs, but now they
are rendered correctly.
