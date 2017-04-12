Incremental
===========

|travis|
|pypi|
|coverage|

Incremental is a small library that versions your Python projects.

API documentation can be found `here <https://hawkowl.github.io/incremental/docs/>`_.


Quick Start
-----------

Add this to your ``setup.py``\ 's ``setup()`` call:

.. code::

   setup(
       use_incremental=True,
       setup_requires=['incremental'],
       install_requires=['incremental'], # along with any other install dependencies
       ...
   }


Then in your project add a ``_version.py`` that contains:

.. code::

   from incremental import Version

   __version__ = Version("widgetbox", 1, 2, 3)
   __all__ = ["__version__"]


Then, so users of your project can find your version, in your project's ``__init__.py`` add:

.. code::

   from ._version import __version__


Subsequent installations of your project will use incremental for versioning.

.. |coverage| image:: https://codecov.io/github/hawkowl/incremental/coverage.svg?branch=master
.. _coverage: https://codecov.io/github/hawkowl/incremental

.. |travis| image:: https://travis-ci.org/hawkowl/incremental.svg?branch=master
.. _travis: http://travis-ci.org/hawkowl/incremental

.. |pypi| image:: http://img.shields.io/pypi/v/incremental.svg
.. _pypi: https://pypi.python.org/pypi/incremental
