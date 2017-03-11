VTK uses shaders to perform its OpenGL rendering.  VTK supports many different
options when it comes to rendering, resulting in potentially thousands of
possible combinations. While we could make one giant shader that uses defines or
uniforms to switch between all these possibilities it would be limiting. Instead
we build up the shader using string replacements on the fly, and then cache the
results for performance.

When writing your own shaders you can use any approach you want. In the end they
are just strings of code. For vtkOpenGLPolyDataMapper we make use of heavy
string replacments.  In other classes we do very little processing as the shader
has far fewer options. Regardless there are a few conventions you should be
aware of.

For shader replacements we tend to use a form of

//VTK::SomeConcept::SomeAction

For example

//VTK::Normal::Dec  - declaration any uniforms/varying needed for normals
//VTK::Normal::Impl - Implementation of shader code for handling normals

All shaders should start with the following line

//VTK::System::Dec

Which vtkOpenGLShaderCache will replace with a #version and some other values to
match the system and OpenGL Context it has. The other line you need (only in
your fragment shader) is

//VTK::Output::Dec

which VTK uses to map shader outputs to the framebufer.

All vertex shaders should name their outputs with a postfix of VSOutput All
geometry shaders should name their outputs with a postfix of GSOutput All
fragment shaders should name their inputs with a postfix of VSOutput.  Put
another way fragment shaders should assuming their input is coming from the
vertex shader.  If a geometry shader is present VTK will rename the fragment
shader inputs from VSOutput to GSOuput automatically.

All variables that represent positions or directions usually have a suffix
indicating the coordinate system they are in. The possible values are

MC  - Model Coordinates
WC  - WC world coordinates
VC  - View Coordinates
DC  - Display Coordinates
NVC - NormalizeViewCoordinates
