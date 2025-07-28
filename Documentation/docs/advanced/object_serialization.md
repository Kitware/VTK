# Object serialization
Modules which have `INCLUDE_MARSHAL` in their `vtk.module` will opt their headers into the automated code generation of (de)serializers. Only classes which are annotated by the `VTK_MARSHALAUTO` wrapping hint will have generated serialization code.

## Marshalling Hints

### Classes

VTK auto generates (de)serialization code in C++ for classes annotated by
the `VTK_MARSHALAUTO` wrapping hint. Additionally, classes that are annotated with
`VTK_MARSHALMANUAL` require manual code. Please refer to [manual-marshal-code](#manual-marshal-code)
which explains everything you need to consider when opting out of the automated marshalling code generation process.

### Properties

#### Excluding properties

You can exclude certain properties of a class by simply annotating the relevant setter/getter functions
with `VTK_MARSHALEXCLUDE(reason)`, where reason is one of `VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL` or
`VTK_MARSHAL_EXCLUDE_REASON_NOT_SUPPORTED`. This reason will be printed in the generated
C++ source code explaining why the property was not serialized.

### Custom get/set functions

Some properties may not be correctly recognized by the property parser because
they have different names for their get and set functions. You can override this
by annotating the get function with the `VTK_MARSHALGETTER(property)` macro. Doing
so will ensure that the function gets recognized as a getter for `property`.
`VTK_MARSHALSETTER(property)` serves a similar purpose.

## Automated code generation
The `vtkWrapSerDes` executable makes use of the `WrappingTools` package to automatically generate:

1. A serializer function with signature:

    ```{note}
    Generated only for classes annotated by `VTK_MARSHALAUTO` macro
    ```

    ```c++
    static nlohmann:json Serialize_vtkClassName(vtkObjectBase*, vtkSerializer*)
    ```

2. A deserializer function with signature:

    ```{note}
    Generated only for classes annotated by `VTK_MARSHALAUTO` macro
    ```

    ```c++
    static void Deserialize_vtkClassName(const nlohmann::json&, vtkObjectBase*, vtkDeserializer*)
    ```

3. A invoker function with signature:

    ```{note}
    Generated for classes annotated by `VTK_MARSHALAUTO` or `VTK_MARSHALMANUAL` macro
    ```

    ```c++
    static nlohmann::json Invoke_vtkClassName(vtkInvoker* invoker, vtkObjectBase* objectBase, const char* methodName, nlohmann::json args)
    ```

4. A registrar function that registers
    - the serializer function with a serializer instance
    - the deserializer function with a deserializer instance
    - the constructor of the VTK class with a deserializer instance
    - Here is an example for the class `vtkObject`:

      ```c++
      int RegisterHandlers_vtkObjectSerDes(void* ser, void* deser, void* invoker)
      {
        int success = 0;
        if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
        {
          if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
          {
            serializer->RegisterHandler(typeid(vtkObject), Serialize_vtkObject);
            success = 1;
          }
        }
        if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
        {
          if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
          {
            deserializer->RegisterHandler(typeid(vtkObject), Deserialize_vtkObject);
            deserializer->RegisterConstructor("vtkObject", []() { return vtkObject::New(); });
            success = 1;
          }
        }
        if (auto* asObjectBase = static_cast<vtkObjectBase*>(invoker))
        {
          if (auto* invokerObject = vtkDeserializer::SafeDownCast(asObjectBase))
          {
            invokerObject->RegisterHandler(typeid(vtkObject), Invoke_vtkObject);
            success = 1;
          }
        }
        return success;
      }
      ```

## Manual marshal code

On the other hand, the `VTK_MARSHALMANUAL` macro is used to indicate that a class
will take part in marshalling, but it cannot trivially (de)serialize it's properties.
This is because one or more of the class's properties may not have an appropriate
setter/getter function that is recognized by the VTK property parser.

For such classes, a developer is expected to provide the code to serialize and deserialize the class in `vtkClassNameSerDesHelper.cxx`
That file must satisfy these conditions:

1. It must live in the same module as `vtkClassName`.
2. It must export a function `int RegisterHandlers_vtkClassNameSerDesHelper(void*, void*, void*)` with C linkage.
3. It must define and declare the `static` functions from [automated-code-generation](#automated-code-generation).
4. Finally, it must implement the helper function `RegisterHandlers_vtkClassNameSerDesHelper` to register your handlers with the object manager. Please see the `RegisterHandlers_vtkObjectSerDes` sample code in [automated-code-generation](#automated-code-generation)
for guidance on implementing this helper.

Example:

```c++
int RegisterHandlers_vtkClassNameSerDesHelper(void* ser, void* deser, void* invoker)
{
  // See RegisterHandlers_vtkObjectSerDes for what must go here.
}
```

that registers:
- a serializer function with a serializer instance
- a deserializer function with a deserializer instance
- a constructor of the VTK class with a deserializer instance
- Optionally, a function that can call methods by name on an object with an invoker instance. The wrapping tool
automatically generates this code even for classes annotated by `VTK_MARSHALMANUAL`. You can overwrite that in
your custom `RegisterHandlers_vtkClassNameSerDesHelper`.

## Marshal hint macro
  1. Classes which are annotated with `VTK_MARSHALAUTO` are considered by the `vtkWrapSerDes` executable.
  2. Classes annotated with `VTK_MARSHALMANUAL` are hand coded in the same module. Here are some examples:
     - `Common/Core/vtkCollectionSerDesHelper.cxx` for `Common/Core/vtkCollection.h`
     - `Common/DataModel/vtkCellArraySerDesHelper.cxx` for `Common/DataModel/vtkCellArray.h`

## Convenient script to annotate headers and module
- The [Utilities/Marshalling/marshal_macro_annotate_headers.py](../../../Utilities/Marshalling/marshal_macro_annotate_headers.py) script annotates headers for automatic or manual serialization. It is fed and driven by the accompanying [Utilities/Marshalling/VTK_MARSHALAUTO.txt](../../../Utilities/Marshalling/VTK_MARSHALAUTO.txt), [Utilities/Marshalling/VTK_MARSHALMANUAL.txt](../../../Utilities/Marshalling/VTK_MARSHALMANUAL.txt) and [Utilities/Marshalling/ignore.txt](../../../Utilities/Marshalling/ignore.txt).

- When the `-u, --update` argument is used, headers are in-place edited to use the `VTK_MARSHAL(AUTO|MANUAL)` wrapping hint. Files that already have this hint are untouched.

- When the `-t, --test` argument is used, the source tree is checked for inconsistent use of marshal macros.
