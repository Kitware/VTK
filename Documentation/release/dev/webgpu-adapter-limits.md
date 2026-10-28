## WebGPU devices use the adapter's advertised limits

WebGPU device creation now requests all advertised core adapter limits. Previously,
only three limits were requested, so larger textures or compute workgroups could
be rejected even when the adapter supported them. The capabilities test now checks
adapter and device limits and retains its full report in CI output.
