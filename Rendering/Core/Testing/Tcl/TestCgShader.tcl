package require vtk
package require vtkinteraction

vtkRenderWindow renWin
vtkRenderWindowInteractor iren
iren SetRenderWindow renWin

vtkRenderer renderer
renWin AddRenderer renderer

vtkSphereSource src1
src1 SetRadius 5
src1 SetPhiResolution 20
src1 SetThetaResolution 20

vtkPolyDataMapper mapper
mapper SetInputConnection [src1 GetOutputPort]

vtkActor actor
actor SetMapper mapper
# Load the material. Here, we are loading a material
# defined in the Vtk Library. One can also specify
# a filename to a material description xml.
[actor GetProperty] LoadMaterial "CgTwisted"

# Turn shading on. Otherwise, shaders are not used.
[actor GetProperty] ShadingOn

# Pass a shader variable need by CgTwisted.
[actor GetProperty] AddShaderVariable "Rate" 1.0

renderer AddActor actor
renWin Render

[renderer GetActiveCamera] Azimuth -50
[renderer GetActiveCamera] Roll 70
renWin Render
wm withdraw .
