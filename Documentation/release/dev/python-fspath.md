# Allow Path objects for SetFileName methods

Starting with Python 3.6, the Python open() method can directly accept
a pathlib.Path object.  This change allows VTK's SetFileName() and
SetDirectoryName() methods to also accept pathlib.Path() objects.
