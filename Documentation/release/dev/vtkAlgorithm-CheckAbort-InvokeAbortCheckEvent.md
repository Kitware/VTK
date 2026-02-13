## vtkAlgorithm::CheckAbort new invokes AbortCheckEvent

vtkAlgorithm::CheckAbort now invokes AbortCheckEvent so that applications
can observe it and react.
