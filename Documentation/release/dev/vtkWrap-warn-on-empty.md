## vtkWrap-warn-on-empty

* `vtkWrap_WarnEmpty` is now available to warn when empty bindings are
  generated.
* The `vtkWrapPython`, `vtkWrapJava`, and `vtkParseJava` tools learned the
  `-Wempty` and `-Wno-empty` flags to control warning when asked to wrap a
  file that results in no new bindings.
