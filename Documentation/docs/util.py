import shutil
import itertools
import os
import re
from pathlib import Path


def gather_module_documentation(
    basepath,
    root_destination,
    custom_paths=[],
    readme_formats=["README.md", "README", "readme.md", "README.txt"],
):
    """For every module directory under basepath (i.e. contains a vtk.module file), copy READMEs under root_destination
    while recreating the directory structure. Additionally look for README under custom_paths.

    A "README" file is any file matching the readme_formats.
    """
    try:
        os.mkdir(root_destination)
    except FileExistsError:
        pass
    paths = Path(basepath).rglob("vtk.module")
    custom_dirs = iter([Path(path) for path in custom_paths])

    paths = itertools.chain(paths, custom_dirs)
    for path in paths:
        if "ThirdParty" in str(path):
            continue
        basename = os.path.dirname(path.absolute())

        # Bring module specific readmes while recreating the data structure
        for readme in readme_formats:
            readme = os.path.join(basename, readme)
            if os.path.exists(readme):
                new_readme = os.path.relpath(readme, start="../../")
                dest = os.path.join(root_destination, new_readme)
                destdir = os.path.dirname(dest)
                if not os.path.exists(destdir):
                    os.makedirs(destdir)
                shutil.copy(readme, dest)


def create_contributing_file():
    """CONTRIBUTING.md has links that cannot be updated with just a
    `relative-docs` since its position in the new documentation tree as well as
    the position of the dependents changed. So we copy the file and replace the
    links by hand."""
    shutil.copy("../../CONTRIBUTING.md", "./developer_guide/contributing.md")
    content_new = None
    with open("./developer_guide/contributing.md", "r") as f:
        content = f.read()
        content_new = re.sub("Documentation/dev/git", "git", content, flags=re.M)
    with open("./developer_guide/contributing.md", "w") as f:
        f.write(content_new)
