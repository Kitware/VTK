# Run a VTK Web Application

    $ cd ${VTK_BUILD}
    $ ./bin/vtkpython Wrapping/Python/vtk/web/${SERVER_SCRIPT} --content www --port ${PORT}
    $ open http://localhost:${PORT}/apps/${APPLICATION}

 Please read ${SERVER_SCRIPT} for the extra arguments usage.

| --------------------------------------------------------------------------- |
| ${APPLICATION}   | ${SERVER_SCRIPT}             | Mandatory Extra arguments |
| ---------------- | ---------------------------- | ------------------------- |
| Cone             | vtk_web_cone.py              |                           |
| PhylogeneticTree | vtk_web_phylogenetic_tree.py | --tree, --table           |
| FileBrowser      | vtk_web_filebrowser.py       | --data-dir                |
| --------------------------------------------------------------------------- |
