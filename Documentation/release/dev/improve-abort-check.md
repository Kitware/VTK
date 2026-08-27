# Improve abort check

VTK now has a more versatile abort checking system.
Instead of using `CheckAbort()`, VTK filters now can
call `CheckAbortAndInvoke()` and `InvokeCleanupCheckAbort()`
in order to trigger event invocations associated with these calls.

While these events have no effect in VTK itself, application
wanting to implement distributed abort can rely on these event to
implement multi process communication in these cases.

The following filters have been moved to the new API:
 - vtkRTAnalyticSource

The `CheckAbort` API is not planned for deprecation for now but ultimely should be once
the new API usage appears more and more.
