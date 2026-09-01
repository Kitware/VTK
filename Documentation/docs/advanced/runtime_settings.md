# Runtime settings

## Selecting between multiple object factories

The `vtkObjectFactory` mechanism supports registering multiple implementations of a class at runtime. By default, VTK uses the first registered implementation, which can cause conflicts if different modules register different implementations of the same class.

To control which implementation is selected, provide a preferences string to `vtkObjectFactory`. The string lists attribute keys with a list of values using the format `key=value1,value2`, and separates attributes with semicolons (e.g., `keyA=valueA1,valueA2;keyB=valueB1,valueB2`).

There are three possible ways to set the preferences are:
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

When preferences are passed to the object factory, the choice of the class to instantiate is based on a score. The factory will assign a score to all classes that derive from the given class name to instantiate. The class with the highest score will be chosen one. The score calculation is explained in the section below.

### Override Attributes

Here is a list of attributes that are recognized in the VTK factory preferences string. These attributes are used as keys in the `VTK_FACTORY_PREFER` environment variable (or equivalently via `vtkObjectFactory::SetPreferences()` or the `--vtk-factory-prefer` command-line option) using the format described above (for example: `Platform=macOS;RenderingBackend=OpenGL`).

| Attribute Name | Possible Values |
|----------------|-----------------|
|Platform|`Embedded`,`iOS`,`macOS`,`WebAssembly`|
|RenderingBackend|`OpenGL`, `WebGPU`, `ANARI`|
|WindowSystem|`Cocoa`,`EGL`,`HTML5`,`OffScreenMESA`,`X11`|
|SupportRenderPass|`true`, `false`|

The class score is computed based on the override attributes above by comparing them against the preference keys and values. Three rules influence the score value:
- If the attribute key is not found in the preferences, the score stays the same.
- If the attribute key is found and the preference value matches, the score is incremented by 1.
- If the attribute key is found and the preference value does not match, the score is decremented by 1.
Once all class scores are computed, the class with the highest score is chosen. If multiple classes have the same highest score value, the order of the preferences value determines which class to pick. Finally, if all scores have a value of 0, the factory will fallback to the default instantiation mechanism.

As an example, if `ANARI` and `OpenGL` are activated on a build, here are some possible score results for a `vtkRenderWindow` instantiation:
- `VTK_FACTORY_PREFER="RenderingBackend=OpenGL,ANARI;SupportRenderPass=true"`
  - `vtkOpenGLRenderWindow` will have a score of 2 because it matches both keys and values.
  - `vtkAnariRenderWindow` will have a score of 1 because it only matches the `RenderingBackend` key.
- `VTK_FACTORY_PREFER="RenderingBackend=OpenGL,ANARI;SupportRenderPass=false"`
  - `vtkOpenGLRenderWindow` will have a score of 1 because it only matches the `RenderingBackend` key.
  - `vtkAnariRenderWindow` will have a score of 2 because it only matches both keys and values.
- `VTK_FACTORY_PREFER="RenderingBackend=ANARI;SupportRenderPass=true"`.
  - The score will be 0 for both render windows because `vtkAnariRenderWindow` matches the first key and `vtkOpenGLRenderWindow` matches the second key. In this case, the factory will fallback to default factory instantiation mechanism.

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
