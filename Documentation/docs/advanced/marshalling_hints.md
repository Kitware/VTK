# Marshalling Hints

## Classes

VTK auto generates (de)serialization code in C++ for classes annotated by
the `VTK_MARSHALAUTO` wrapping hint. Additionally, classes that are annotated with
`VTK_MARSHALMANUAL` require manual code. Please refer to [manual-marshal-code](/advanced/object_serialization.html#manual-marshal-code)
which explains everything you need to consider when opting out of the automated marshalling code generation process.

## Properties

### Excluding properties

You can exclude certain properties of a class by simply annotating the relevant setter/getter functions
with `VTK_MARSHALEXCLUDE(reason)`, where reason is one of `VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL` or
`VTK_MARSHAL_EXCLUDE_REASON_NOT_SUPPORTED`. This reason will be printed in the generated
C++ source code explaining why the property was not serialized.

## Custom get/set functions

Some properties may not be correctly recognized by the property parser because
they have different names for their get and set functions. You can override this
by annotating the get function with the `VTK_MARSHALGETTER(property)` macro. Doing
so will ensure that the function gets recognized as a getter for `property`.
`VTK_MARSHALSETTER(property)` serves a similar purpose.
