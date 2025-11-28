# Support for Object Factory Preferences

You can now specify preferred implementations for certain classes, enabling finer control over which objects are instantiated at runtime.
See the [Object Factory](https://vtk.org/doc/nightly/html/classvtkObjectFactory.html#details) documentation for more information on how to use this feature.
This feature allows you to set preferences for specific classes, ensuring that your application uses the desired implementation when multiple options are available.

This feature closes the issue [vtk/vtk#17218](https://gitlab.kitware.com/vtk/vtk/-/issues/17218).
