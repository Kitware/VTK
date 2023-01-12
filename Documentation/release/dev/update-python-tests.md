## Add script to modernize older VTK Python scripts

The script ModernizePythonImports.py in VTK/Utilities/Maintenance/
will parse Python scripts and replace "import vtk" with importation
of used classes from specific VTK extension modules.  This improves
the efficiency of loading, since only needed modules are imported.
