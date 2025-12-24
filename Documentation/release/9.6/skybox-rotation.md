## Add rotation parameters to opengl skybox

OpenGL Skybox now supports rotations, which is also applied to the image-based lighting in the polydata mapper.

A rotation matrix `EnvironmentRotationMatrix` is now available in the vtkRenderer, which replaces EnvironmentRight
and EnvironmentUp members. These two members have been moved to private members, but not removed for backward compatibility purposes.
They may removed if some refactoring is done on the getter/setters (see the getters documentation).
