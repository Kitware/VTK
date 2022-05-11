from pathlib import Path
import subprocess

dist_path = Path('dist')
install_path = Path('install')

wheel_file = next(dist_path.glob('*.whl'))
sdk_name = wheel_file.stem.replace('vtk-', 'vtk-wheel-sdk-')
tarball_name = f'{sdk_name}.tar.xz'

install_path.rename(sdk_name)
try:
    cmd = ['cmake', '-E', 'tar', 'cJf', tarball_name, sdk_name]
    subprocess.check_call(cmd)
finally:
    Path(sdk_name).rename(install_path)
