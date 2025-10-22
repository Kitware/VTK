### ANARI Renderer Introspection, Robust Device Management, and API Refinements

This release significantly enhances the ANARI rendering backend by empowering
users with greater control and insight into ANARI renderer parameters,
alongside improvements to internal device management and API consistency. These
changes lay the groundwork for more dynamic and user-friendly ANARI integration
within VTK applications.

* Developers can now programmatically discover and inspect the full range of
parameters supported by any ANARI renderer subtype. This capability allows
building robust, adaptable user interfaces and applications that can
dynamically adjust to the capabilities of different ANARI renderers. The
`vtkAnariRenderer` class now exposes a rich set of methods:
    *   `GetRendererParameters()`: Returns a list of all supported parameter
    names and their ANARI types, allowing for dynamic enumeration.
    *   `GetRendererParameterDescription(param)`: Provides a human-readable
    description for any given parameter, aiding in user understanding.
    *   `IsRendererParameterRequired(param)`: Indicates whether a parameter is
    mandatory for the renderer's operation, facilitating validation.
    *   `GetRendererParameterDefault(param)`: Retrieves the default value for a
    parameter, useful for resetting or understanding initial states.
    *   `GetRendererParameterMinimum(param)`: Returns the minimum allowed value
    for numerical parameters, enabling bounds checking.
    *   `GetRendererParameterMaximum(param)`: Returns the maximum allowed value
    for numerical parameters, also for bounds checking.
    *   `GetRendererParameterValue(param)`: Fetches the currently set value of
    a parameter on the active ANARI renderer instance. This new introspection
    capability significantly reduces the need to consult external ANARI
    documentation for specific renderer implementations, promoting a more
    self-describing and discoverable API.

* A new method, `vtkAnariDevice::GetAnariRendererSubTypes()`, has been
introduced. This allows applications to query the ANARI device for all
available renderer subtypes it can instantiate. This is crucial for
applications that need to offer users a choice of renderers, adapting to
different performance characteristics or visual styles without hardcoding
options.

* The `vtkAnariRenderer` class has been refactored to manage its associated
ANARI device via a `vtkSmartPointer<vtkAnariDevice>` instead of a raw
`anari::Device` handle. This change fundamentally improves resource management
by:
    *   Ensuring proper ownership and lifetime management of the
    `vtkAnariDevice` instance.
    *   Automating cleanup and release of ANARI device resources when
    `vtkAnariRenderer` objects are destroyed.
    *   Aligning `vtkAnariRenderer` with standard VTK object management
    paradigms, reducing the potential for memory leaks and dangling pointers.
    The `SetAnariDevice` method now accepts a `vtkAnariDevice*`, and a new
    `vtkGetSmartPointerMacro` accessor, `GetAnariDevice()`, provides safe
    access to the managed device.

* The callback mechanism for newly created ANARI devices in `vtkAnariDevice`
has been simplified. The `OnNewDeviceCallback` now has a signature of
`std::function<void()>` (instead of `std::function<void(anari::Device)>`),
implying that the callback should access the device directly from the
`vtkAnariDevice` instance. This reduces direct `anari::Device` handle passing
in callbacks, promoting a more centralized and encapsulated approach to device
access, as seen in `vtkAnariPass`.

* Parameter order for `useDebugDevice` in `vtkAnariDeviceInternals::InitAnari`
and its wrapper `vtkAnariDevice::SetupAnariDeviceFromLibrary` has been
standardized for clarity and consistency.

* A new test, `TestAnariRendererParameters.cxx`, has been added to rigorously
validate the ANARI renderer parameter introspection capabilities, ensuring that
all new methods function as expected across different ANARI device and renderer
configurations.
