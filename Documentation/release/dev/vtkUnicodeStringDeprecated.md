## Deprecated vtkUnicodeString and vtkUnicodeStringArray

A UTF-8 everywhere policy has been adopted in VTK since version 8.2, so char*/strings are now expected to contain only UTF-8 encoded bytes. Consequently the functionality provided by vtkUnicodeString and vtkUnicodeStringArray is redundant. All methods using those classes have been marked as deprecated and will be removed in VTK 9.1.

The only functionality that will be lost after removing vtkUnicodeString will be the ability to explicitly convert UTF-8 <=> UTF-16 text. No code within VTK requires such conversion except where that is provided by KWSYS for Windows API calls.

Text data exported by vtkUnicodeStringArray has always been output as UTF-8 bytes so this can be reliably loaded using vtkStringArray.
