from __future__ import annotations

import importlib.metadata

import vtk_sdk as m


def test_version():
    assert importlib.metadata.version("vtk_sdk") == m.__version__
