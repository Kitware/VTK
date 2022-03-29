## Deleted vtkUnicodeString and vtkUnicodeStringArray

Since VTK 8.2 a UTF-8 everywhere policy has been adopted so that char*/strings are now expected to contain only UTF-8 encoded bytes. Consequently the functionality provided by vtkUnicodeString and vtkUnicodeStringArray is no longer needed. All methods using those classes have been removed.

The only functionality lost due to the elimination of vtkUnicodeString is the ability to explicitly convert UTF-8 <=> UTF-16 text. No code within VTK requires such conversion except where that is provided by KWSYS for Windows API calls.

Any text data exported by vtkUnicodeStringArray will have been output as UTF-8 bytes so this can be reliably read using vtkStringArray.
