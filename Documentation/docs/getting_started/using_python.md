# Using Python

VTK is available on [PyPi](https://pypi.org/) for Windows,Mac OS and Linux.
```
pip install vtk
```
or in a [virtual environment](https://docs.python.org/3/library/venv.html) if you want to install the package only locally instead of system-wide

::::{tab-set}

:::{tab-item} Linux

```
python -m venv ./env
source ./env/bin/activate
pip install vtk
```
:::

:::{tab-item} macOS

```
python -m venv ./env
source ./env/bin/activate
pip install vtk
```
:::

:::{tab-item} Windows
Using `PowerShell`
```
python -m venv env
.\env\Activate.ps1
pip install vtk
```

or using `cmd.exe`
```
python -m venv env
.\env\activate.bat
pip install vtk
```
:::

::::

To verify the installation try to import vtk from an interactive python environment:
```python
>>> import vtk
>>> print(vtk.__version__)
9.2.6
```

That's it ! You may now try some of the
[tutorials](https://kitware.github.io/vtk-examples/site/Python/#tutorial),
[how to guides](https://kitware.github.io/vtk-examples/site/PythonHowTo) or
[examples](https://kitware.github.io/vtk-examples/site/Python).
