## `vtkPSystemTools::FindProgramPath` and prefix support

* The `vtkPSystemTools::FindProgramPath` now ignores the `buildDir` and
  `installPrefix` arguments. KWSys has dropped this API and the only extant
  callers of the function never set these parameters, so ignore them. Mention
  them in the error message if they are provided though.
