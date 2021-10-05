## Remove nested parallelism from vtkCheckerboardSplatter

vtkCheckerboardSplatter used to have nested parallelism that caused issues when std::thread was used as the SMP back-end.
The nested parallelism has been removed and the test has been re-enabled, which resolvess the related task of #18222.
