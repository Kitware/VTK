## Fix harmless EGL render window initialization warning

VTK now removes unused OpenGL point sprite enabling code from the EGL render window initialization. Point sprites are available by default in core OpenGL profiles and do not require explicit enabling. This removes a harmless warning during window setup on some systems.
