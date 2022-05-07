from pathlib import Path
import subprocess

dist_path = Path('dist')
install_path = Path('install')

wheel_file = next(dist_path.glob('*.whl'))
sdk_name = wheel_file.stem.replace('vtk-', 'vtk-wheel-sdk-')
tarball_name = f'{sdk_name}.tar.xz'

install_path.rename(sdk_name)
try:
    subprocess.check_call(['tar', 'cJf', tarball_name, sdk_name])
finally:
    Path(sdk_name).rename(install_path)
