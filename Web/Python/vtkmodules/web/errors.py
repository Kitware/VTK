WEB_DEPENDENCY_MISSING_MESSAGE = """Please install VTK's Web module dependencies.

These include `wslink` and can be easily installed  with vtk by using the
`web` extra requirements option. For example:

    pip install vtk[web]

"""

class WebDependencyMissingError(ImportError):
    def __init__(self, message=WEB_DEPENDENCY_MISSING_MESSAGE):
        super().__init__(message)
