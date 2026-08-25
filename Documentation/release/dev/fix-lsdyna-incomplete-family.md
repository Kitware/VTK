## vtkLSDynaReader handles incomplete and multi-solver d3plot databases

vtkLSDynaReader no longer crashes when a d3plot database is incomplete
(for example when only the root geometry file of a family is present or
when the last state is truncated) or when the first family file ends
with sections the reader does not parse. Such trailing data was
previously announced as a valid time step and reading it aborted the
application.

The reader also recognizes the multi-solver sentinel value in the
NCFDV1 control word. Databases written with multi-solver output no
longer report a phantom species array or misaligned states; the
structural mesh is read and a warning explains that the extra
solver-mesh datasets are not supported yet.
