## URI support in VTK: vtkURI and vtkURILoader

VTK now support [URI](https://datatracker.ietf.org/doc/html/rfc3986) parsing, resolution and loading through the two new classes `vtkURI` and `vtkURILoader`.

URI support as been implemented to enable resource stream support in readers that need to access multiple resources.
This is the case of [glTF](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html) format,
which uses ["data" URIs](https://datatracker.ietf.org/doc/html/rfc2397) and relative references from the main file to load binary buffers.

The `vtkURILoader` is intended to be as generic as possible. It should be possible to use it everywhere an URI may be encountered.
Note that "relative file paths" are valid [URI relative references](https://datatracker.ietf.org/doc/html/rfc3986#section-4.2), and can be generalized to other sources, such as web resources.

For more infomation about URI usage and loading, please refer to the `vtkURILoader` documentation.
