import setuptools # importing this suppresses deprecation methods
# XXX(python-3.12): figure out what replaces this kind of access after
# `distutils` is removed.
from distutils.dist import Distribution
from distutils.command import build

d = Distribution()
b = build.build(d)
b.finalize_options()
print(b.build_platlib)
