## Remove variables related to Scalar and Vector data from public API in vtkFLUENTCFFReader

`ScalarDataChunk` and `VectorDataChunk` struct have been refactored into a single generic struct `DataChunk` which is private.
These following variables and methods have been removed from the public API of `vtkFLUENTCFFReader` :
- `struct ScalarDataChunk`
- `struct VectorDataChunk`
- `ScalarDataChunks`
- `VectorDataChunks`
- `PreReadScalarData`
- `PreReadVectorData`
- `NumberOfVectors`
- `NumberOfScalars`
