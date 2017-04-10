# Updating Third Party Projects

When updating a third party project, any changes to the imported project
itself (e.g., the `zlib/vtkzlib` directory for zlib), should go through the
`update.sh` framework. This framework ensures that all patches to the third
party projects are tracked externally and available for (preferably) upstream
or other projects also embedding the library.

The projects listed in the [imported.md](imported.md) file are already using
this setup.

Any updates to projects not listed there should first convert over to this
framework.

# Updating a Project

Once converted, a project should be updated by applying patches to the
repository specified in its `update.sh` script. Once the changes are merged,
pulling the changes involves running the `update.sh` script. This will update
the local copy of the project to the version specified in `update.sh` (usually
a `for/foo` branch, but may be `master` or any other Git reference) and merge
it into the main tree.

This requires a Git 2.5 or higher due the `worktree` tool being used to
simplify the availability of the commits to the main checkout.

# Porting a Project

When converting a project, if there are any local patches, a project should be
created [on GitLab](https://gitlab.kitware.com/third-party) to track it. If
the upstream project does not use Git, it should be imported into Git (there
may be existing conversions available on Github already). The project's
description should indicate where the source repository lives.

Once a mirror of the project is created, a branch named `for/foo` should be
created where patches for the `foo` project will be applied (i.e., `for/vtk`
for VTK's patches to the project). Usually, changes to the build system, the
source code for mangling, the addition of `.gitattributes` files, and other
changes belong here. Functional changes should be submitted upstream (but may
still be tracked so that they may be used).

The basic steps to import a project `twisted` based on the tag
`twisted-17.1.0` looks like this:

```sh
$ git clone https://github.com/twisted/twisted.git
$ cd twisted/
$ git remote add kitware git@gitlab.kitware.com:third-party/twisted.git
$ git push -u kitware
$ git push -u kitware --tags
$ git checkout twisted-17.1.0
$ git checkout -b for/vtk
$ git push --set-upstream kitware for/vtk
```

Making the initial import involves filling out the project's `update.sh`
script in its directory. The [update-common.sh](update-common.sh) script
describes what is necessary, but in a nutshell, it is basically metadata such
as the name of the project and where it goes in the importing project.

The most important bit is the `extract_source` function which should subset
the repository. If all that needs to be done is to extract the files given in
the `paths` variable (described in the `update-common.sh` script), the
`git_archive` function may be used if the `git archive` tool generates a
suitable subset.

Make sure `update.sh` is executable before commit. On Unix, run:

```sh
  $ chmod u+x update.sh && git add -u update.sh
```

On Windows, run:

```sh
  $ git update-index --chmod=+x update.sh
```

Also add an entry to [imported.md](imported.md) for the project, and
`CMakeLists.txt` and `module.cmake` as appropriate.

# Process

The basic process involves a second branch where the third party project's
changes are tracked. This branch has a commit for each time it has been
updated and is stripped to only contain the relevant parts (no unit tests,
documentation, etc.). This branch is then merged into the main branch as a
subdirectory using the `subtree` merge strategy.

Initial conversions will require a manual push by the maintainers since the
conversion involves a root commit which is not allowed under normal
circumstances. Please send an email to the mailing list asking for assistance
if necessary.
