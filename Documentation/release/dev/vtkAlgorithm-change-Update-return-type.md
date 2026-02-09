## vtkAlgorithm::Update() change return type to bool

vtkAlgorithm::Update*() used to return `void`, now they returns `bool`.
- UpdateInformation()
- UpdateDataObject()
- UpdateWholeExten()
- Update()

For anyone overriding this method, this is a breaking change and require a modification in VTK consumer code.
