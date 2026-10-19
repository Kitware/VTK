"""Give the vtk-sdk wheel the same tags as the vtk wheel.

Usage: retag_sdk_wheel.py <vtk wheel> <vtk-sdk wheel>
"""

from pathlib import Path
import subprocess
import sys

from packaging.utils import parse_wheel_filename

vtk_wheel = Path(sys.argv[1])
sdk_wheel = Path(sys.argv[2])

# A wheel can carry several tags per field (e.g. manylinux_2_17 and
# manylinux_2_28); `wheel tags` takes them joined with a ".".
tags = parse_wheel_filename(vtk_wheel.name)[3]


def joined(attr):
    return '.'.join(sorted({getattr(tag, attr) for tag in tags}))


subprocess.check_call([
    sys.executable, '-m', 'wheel', 'tags',
    '--python-tag', joined('interpreter'),
    '--abi-tag', joined('abi'),
    '--platform-tag', joined('platform'),
    '--remove', str(sdk_wheel),
])
