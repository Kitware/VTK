import sys
import sysconfig

justver = None
if sys.platform == 'linux':
    justver = (3, 6)
elif sys.platform == 'win32':
    justver = (3, 9)
elif sys.platform == 'darwin':
    justver = (3, 9)

if justver is not None and sys.version_info[:2] <= justver:
    tag = '{}.{}'.format(sys.version_info.major, sys.version_info.minor)
else:
    tag = '{}-{}{}'.format(sys.implementation.name, sys.version_info.major, sys.version_info.minor)

manual = 'build/lib.{}-{}'.format(sysconfig.get_platform(), tag)

try:
    import setuptools # importing this suppresses deprecation methods
    # XXX(python-3.12): figure out what replaces this kind of access after
    # `distutils` is removed.
    from distutils.dist import Distribution
    from distutils.command import build

    d = Distribution()
    b = build.build(d)
    b.finalize_options()
    # Fix Windows slashes.
    api = b.build_platlib.replace('\\', '/')

    if not manual == api:
        sys.stderr.write('mismatch with manual computation: "{}" vs. "{}"\n'.format(manual, api))
except ImportError:
    pass

print(manual)
