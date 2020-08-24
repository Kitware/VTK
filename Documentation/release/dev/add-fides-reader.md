## Add vtkFidesReader

You can now read ADIOS2 files or data streams using [Fides](https://gitlab.kitware.com/vtk/fides).
You can either provide a JSON file containing the data model or allow Fides to generate a data model for you (see Fides documentation for details).

Fides converts the ADIOS2 data to a VTK-m dataset and the vtkFidesReader creates partitioned datasets that contain either native VTK datasets or VTK VTK-m datasets.
Time and time streaming is supported. Note that the interface for time streaming is different.
It requires calling PrepareNextStep() and Update() for each new step.
