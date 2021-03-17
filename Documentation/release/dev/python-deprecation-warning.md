# New DeprecationWarning from Python

If a deprecated VTK method is called from Python, then a DeprecationWarning
will be generated from Python.  Python silences DeprecationWarning by default,
so you must add the following to any Python programs that you want to check
for deprecated method usage:

    import warnings
    warnings.filterwarnings("default", category=DeprecationWarning)

If there is a new method that replaces the deprecated method, the name of
the new method will be stated in the warning text.
