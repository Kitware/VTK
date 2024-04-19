# VTK::RenderingVR
## vtkRenderingVR - Virtual reality support for VTK

### Introduction

The VR module defines an API and support classes for adding virtual reality
support to VTK. The OpenVR and OpenXR modules are both subclassed off of
this module. For a list of todos and development issues please see

https://gitlab.kitware.com/vtk/vtk/-/issues/18302

### Supported Devices

The VR module aims to support runtimes that implement the OpenXR or OpenVR
standards.

See [`VTK::RenderingOpenXR` documentation](../OpenXR/README.md) for information on
rendering with the modern OpenXR specification.

See [`VTK::RenderingOpenVR` documentation](../OpenVR/README.md) for information on
rendering with the legacy OpenVR specification.

### Coordinate Systems

With VR the transformations between coordinate systems can quickly become
confusing. To help with this note that most matrices in the VR code are
stored in vtk convention. That is `a = Mx` where x is a column vector in
homogeneous coordinates. Matrices are named according to what spaces they
transform between. For example PhysicalToLeftEyeMatrix. Some common
coordinate systems are listed below in order of coordinate flow.

Note that in vtkMatrix4x4 multiplcations are done from right to left
so to compute a matrix from spaces A to C you would do

`vtkMatrix4x4::Multiply4x4(BtoCMatrixInput, AtoBMatrixInput, AtoCMatrixOutput)`

Model -> World -> Physical -> Left/RightEye -> Projection

- Model - what an actor's data is in
- World - common coordinate system for all actors
- Physical - the physical VR space in meters with 0,0,0 being the center of
  the floor of the room
- Device - the viewpoint (position and orientation) of a device such as a
  controller
- LeftEye (and RightEye) - the viewpoint of the left and right eye
- Projection - in clip space, the expected output space for vertex shaders

The matrices that go between these spaces are as follows and they can be
inverted as desired. You will also find some additional matrices that
combine some of these transformations into a single matrix for convenience
such as VRHMDCamera->WorldToLeftEyeMatrix.

- Model -> World = the actor's matrix
- World -> Physical = inverse of VRRenderWindow->GetPhysicalToWorldMatrix()
- Physical -> LeftEye = VRHMDCamera->PhysicalToLeftEyeMatrix
- LeftEye -> Projection = VRHMDCamera->LeftEyeToProjectionMatrix
- Physical -> Device = inverse of
  VRRenderWindow->GetDeviceToPhysicalMatrixForDevice()

There are some other matrices used in the camera that are stored in OpenGL
format (transpose of VTK format) using an older naming convention. These are
names such as WCDCMatrix, the names correspond to

- MC = model coordinates (same as above)
- WC = world coordinates (same as above)
- VC = view coordinates, world coordinates translated and rotated to the
  camera, similar to the LeftEye space
- DC = device coordinates (device in this context is a GPU, so same as
  projection coordinates above)
