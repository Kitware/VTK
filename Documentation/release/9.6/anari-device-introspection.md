### ANARI Renderer Introspection

The `vtkAnariRenderer` class now allows users to introspect and query the
parameters supported by the chosen ANARI rendering backend. New methods have
been added to query the parameters, their types, default values, minimum,
maximum, and descriptions. This enables applications to dynamically adapt to
different ANARI renderers without hardcoding parameter knowledge.
