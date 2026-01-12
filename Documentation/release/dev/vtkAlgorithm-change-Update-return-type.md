## vtkAlgorithm::Update() change return type to bool

vtkAlgorithm::Update() used to return `void`, it now returns `bool`.
For anyone overriding this method, this is a breaking change and require a modification in VTK consumer code.
