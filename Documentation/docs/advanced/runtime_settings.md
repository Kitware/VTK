# Runtime settings

## Selecting between multiple object factories

The `vtkObjectFactory` mechanism supports registering multiple implementations of a class at runtime. By default, VTK uses the first registered implementation, which can cause conflicts if different modules register different implementations of the same class.

To control which implementation is selected, provide a preferences string to `vtkObjectFactory`. The string lists attribute keys with a list of values ordered by priority, using the format `key=value1,value2`, and separates attributes with semicolons (e.g., `keyA=valueA1,valueA2;keyB=valueB1,valueB2`).

In the below examples, the vtkObjectFactory prefers implementations that have `valueA1` over `valueA2` for an attribute `keyA`, and if that fails, `valueB1` over `valueB2` for an attribute `keyB`:

The three possible ways to set the preferences are:
1. Set the environment variable `VTK_FACTORY_PREFER` before starting your application.
    ```
    VTK_FACTORY_PREFER="keyA=valueA1,valueA2;keyB=valueB1,valueB2"
    ```
2. Call the static method `vtkObjectFactory::SetPreferences()` before creating any VTK objects.

    ```cpp
    vtkObjectFactory::SetPreferences("keyA=valueA1,valueA2;keyB=valueB1,valueB2");
    ```
3. Call the static method `vtkObjectFactory::InitializePreferencesFromCommandLineArgs(argc, argv)` early in your application, passing the command line arguments.

    ```cpp
    int main(int argc, char* argv[])
    {
      vtkObjectFactory::InitializePreferencesFromCommandLineArgs(argc, argv);
      // ...
    }
    ```

    ```sh
    # Example preference string ('=' sign can be omitted and replaced with a space)
    ./my_vtk_app --vtk-factory-prefer="keyA=valueA1,valueA2;keyB=valueB1,valueB2"
    ```

### Override Attributes

Here is a list of attributes that are recognized in the VTK factory preferences string. These attributes are used as keys in the `VTK_FACTORY_PREFER` environment variable (or equivalently via `vtkObjectFactory::SetPreferences()` or the `--vtk-factory-prefer` command-line option) using the format described above (for example: `Platform=macOS;RenderingBackend=OpenGL`).

| Attribute Name | Possible Values |
|----------------|-----------------|
|Platform|`Embedded`,`iOS`,`macOS`,`WebAssembly`|
|RenderingBackend|`OpenGL`, `WebGPU`|
|WindowSystem|`Cocoa`,`EGL`,`HTML5`,`OffScreenMESA`,`X11`|

## OpenGL

On Linux and Windows, VTK will attempt to detect support for an OpenGL context backend at runtime
and create an appropriate subclass of `vtkOpenGLRenderWindow`. You can override this process by
specifying an environment variable `VTK_DEFAULT_OPENGL_WINDOW`. The possible values
are:

  1. `vtkXOpenGLRenderWindow` (Linux; applicable only when `VTK_USE_X` is `ON`, which is the default setting)
  2. `vtkWin32OpenGLRenderWindow` (Windows; applicable only when `VTK_USE_WIN32_OPENGL` is `ON`, which is the default setting)
  3. `vtkEGLRenderWindow` (applicable only when `VTK_OPENGL_HAS_EGL` is `ON`, which is the default setting)
  4. `vtkOSOpenGLRenderWindow` (OSMesa, requires that `osmesa.dll` or `libOSMesa.so` is installed)

Note: VTK does **not** support OSMesa on macOS, iOS, Android and WebAssembly platforms.

### Multisample anti-aliasing

Some OpenGL drivers have rendering problems when Multisample anti-aliasing is enabled.
It is possible to specify the environment variable `VTK_FORCE_MSAA` to troubleshoot rendering problems with these values:

  1. `0` to disable MSAA
  2. `1` to enable it regardless even when the driver is known to have problems with MSAA
