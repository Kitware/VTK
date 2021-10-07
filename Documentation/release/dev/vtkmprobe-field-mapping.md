# vtkmProbe filter field mapping

Reflecting the changes in the Probe filter in the vtk-m project, now in
`vtkmProbe`, all probed fields of any type are returned as point fields in output,
meaning that given:

```
source->GetPointData()->AddArray(pointArray);
source->GetCellData()->AddArray(cellArray);
...
probe->SetInputData(input);
probe->SetSourceData(source);
...
vtkDataSet* result = probe->GetOutput();
```

To access the probed elements in `cellArray` instead of doing:

```
result->GetCellData()->GetArray(cellArray->GetName());
```
We now access it with:

```
result->GetPointData()->GetArray(cellArray->GetName());
```
