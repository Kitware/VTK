## Envirnoment variable in vtkSMPTools Initialize

`vtkSMPTools::Initialize` is now capable of changing the (maximum) number of threads at runtime, using the `VTK_SMP_MAX_THREADS` environment variable.

The user still need to call `vtkSMPTools::Initialize()` without any parameters.
