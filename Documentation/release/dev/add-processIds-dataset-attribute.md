## Add ProcessIds dataset attribute

You can now access the ProcessIds data array directly from `vtkDataSetAttributes`
just like any other data array (e.g GlobalIds or Normals).

`vtkProcessIdScalars` is deprecated in favor of `vtkGenerateProcessIds`.

`vtkGenerateProcessIds` allows you to generate process ids for both PointData
and CellData, and store it as a ProcessIds attribute.

### Example

Here is an example of how to use the new filter to replace the deprecated one.

From:
```cpp
vtkNew<vtkProcessIdScalars> processIdsGenerator;
processIdsGenerator->SetInputConnection(someData->GetOutputPort());
processIdsGenerator->SetScalarModeToCellData();
processIdsGenerator->Update();

vtkDataSet* pidGeneratorOutput = processIdsGenerator->GetOutput();
vtkIntArray* pidArray = vtkIntArray::SafeDownCast(pidGeneratorOutput->GetCellData()->GetArray("ProcessId"));
```

To:
```cpp
vtkNew<vtkGenerateProcessIds> processIdsGenerator;
processIdsGenerator->SetInputConnection(someData->GetOutputPort());
processIdsGenerator->GeneratePointDataOff();
processIdsGenerator->GenerateCellDataOn();
processIdsGenerator->Update();

vtkDataSet* pidGeneratorOutput = processIdsGenerator->GetOutput();
vtkIdTypeArray* pidArray = vtkIdTypeArray::SafeDownCast(pidGeneratorOutput->GetCellData()->GetProcessIds());
```
