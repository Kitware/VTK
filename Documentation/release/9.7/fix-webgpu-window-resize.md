## WebGPU window resize now re-renders correctly

VTK's WebGPU rendering backend now properly handles window resize events. Previously, resizing the window would stretch the rendered image instead of re-rendering the scene at the new resolution. The fix validates surface texture dimensions and handles outdated surface status to ensure proper reconfiguration during resize operations.
