## vtkExtractEdges: Improve Performance

`vtkExtractEdges` used to be multithreaded only when `UseAllPoints` was true, which by default is false. Now it is
multithreaded both when `UseAllPoints` is `true` and when it is `false`, and therefore you are expected to see a
significant performance improvement when `UseAllPoints` is `false` (the default).
