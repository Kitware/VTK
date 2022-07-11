## `vtkStdString` implicit `const char*` is deprecated

The `vtkStdString` implicit conversion to `const char*` is deprecated. Instead,
call `.c_str()` explicitly on the instance.
