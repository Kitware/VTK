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

if hasattr(sys, "_is_gil_enabled") and not sys._is_gil_enabled():
    tag += "t"

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
        caveat = ''
        st_vers = tuple(map(int, setuptools.__version__.split('.')))
        check_vers = (62, 1, 0)
        if st_vers < check_vers:
            st_vers_str = '.'.join(map(str, st_vers))
            check_vers_str = '.'.join(map(str, check_vers))
            caveat = '; consider installing setuptools>={} (currently {})'.format(check_vers_str, st_vers_str)

        sys.stderr.write('mismatch with manual computation: "{}" (computed) vs. "{}" (distutils){}\n'.format(manual, api, caveat))
except ImportError:
    pass

print(manual)
