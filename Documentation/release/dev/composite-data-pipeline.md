# vtkCompositeDataPipeline: Check for AbortExecute

vtkCompositeDataPipeline now checks for AbortExecute before executing filters.
This makes it possible to abort when processing composite data in simple filters.
