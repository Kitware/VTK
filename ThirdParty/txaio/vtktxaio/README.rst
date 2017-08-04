txaio
=====

| |Version| |Build Status| |Coverage| |Docs|

--------------

**txaio** is a helper library for writing code that runs unmodified on
both `Twisted <https://twistedmatrix.com/>`_ and `asyncio <https://docs.python.org/3/library/asyncio.html>`_ / `Trollius <http://trollius.readthedocs.org/en/latest/index.html>`_.

This is like `six <http://pythonhosted.org/six/>`_, but for wrapping
over differences between Twisted and asyncio so one can write code
that runs unmodified on both (aka *source code compatibility*). In
other words: your *users* can choose if they want asyncio **or** Twisted
as a dependency.

Note that, with this approach, user code **runs under the native event
loop of either Twisted or asyncio**. This is different from attaching
either one's event loop to the other using some event loop adapter.


Platform support
----------------

**txaio** runs on CPython 2.7/3.3+ and PyPy 2, on top of Twisted or asyncio. Specifically, **txaio** is tested on the following platforms:

* CPython 2.7 on Twisted 12.1, 13.2, 15.4, 16.5, trunk and on Trollius 2.0
* CPython 3.3 on Twisted 15.4, 16.5, trunk and on Trollius 2.0
* CPython 3.4 on Twisted 15.4, 16.5, trunk and on asyncio (stdlib)
* CPython 3.5 on Twisted 15.4, 16.5, trunk and on asyncio (stdlib)
* PyPy 2 on Twisted 12.1, 13.2, 15.4, 16.5, trunk and on Trollius 2.0


How it works
------------

Instead of directly importing, instantiating and using ``Deferred``
(for Twisted) or ``Future`` (for asyncio) objects, **txaio** provides
helper-functions to do that for you, as well as associated things like
adding callbacks or errbacks.

This obviously changes the style of your code, but then you can choose
at runtime (or import time) which underlying event-loop to use. This
means you can write **one** code-base that can run on Twisted *or*
asyncio (without a Twisted dependency) as you or your users see fit.

Code like the following can then run on *either* system:

.. sourcecode:: python

    import txaio
    txaio.use_twisted()  # or .use_asyncio()

    f0 = txaio.create_future()
    f1 = txaio.as_future(some_func, 1, 2, key='word')
    txaio.add_callbacks(f0, callback, errback)
    txaio.add_callbacks(f1, callback, errback)
    # ...
    txaio.resolve(f0, "value")
    txaio.reject(f1, RuntimeError("it failed"))

Please refer to the `documentation <https://txaio.readthedocs.io/en/latest/>`_ for description and usage of the library features.


.. |Version| image:: https://img.shields.io/pypi/v/txaio.svg
   :target: https://pypi.python.org/pypi/txaio

.. |Build Status| image:: https://travis-ci.org/crossbario/txaio.svg?branch=master
   :target: https://travis-ci.org/crossbario/txaio

.. |Coverage| image:: https://codecov.io/github/crossbario/txaio/coverage.svg?branch=master
   :target: https://codecov.io/github/crossbario/txaio

.. |Docs| image:: https://readthedocs.org/projects/txaio/badge/?version=latest
   :target: https://txaio.readthedocs.io/en/latest/
