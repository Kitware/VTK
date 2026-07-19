## WebGPU render window no longer marshals its configuration

`vtkWebGPURenderWindow` excludes the `WGPUConfiguration` property from
marshalling. The property describes a live device/adapter, and replaying it
into an initialized window through deserialization destroyed and recreated the
WebGPU device, leaving the window unable to render.

`SetWGPUConfiguration` also returns early when the given configuration is
already in use instead of tearing down and reinitializing the device.
