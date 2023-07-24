# Updating Third Party Projects

When updating a third party project, any changes to the imported project
itself (e.g., the `zlib/vtkzlib` directory for zlib), should go through the
`update.sh` framework. This framework ensures that all patches to the third
party projects are tracked externally and available for (preferably) upstream
or other projects also embedding the library.

The [imported.md](imported.md) document lists all projects grouped by import
method:
1. `update.sh` framework
2. `git submodule`
3. `copy`

:::{important}
Any updates to projects imported through the `copy` method should first be converted
over to the `update.sh` framework.
:::

## Updating a Project Upstream

Ideally, any code changes to third party code should first be submitted to the upstream
project using whatever workflow they prefer or require.  Once that is done, the changes
can next be brought into VTK.

## Updating the Import

Examine the project's `update.sh` script and note the value of the `repo=` field.

If it's referring to anything other than [Kitware's GitLab](https://gitlab.kitware.com/third-party),
then skip to the next section.

Otherwise, you first need to bring in the upstream changes into the `third-party` repo.
To do that, first fork and clone the repository named in the `repo=` field.
Then use git commands to bring in a copy of the upstream changes.

Here's an example of updating the `twisted` project from tag 17.1.0 to 17.5.0:

```sh
$ cd twisted/
$ git checkout for/vtk
$ git fetch origin
$ git rebase --onto twisted-17.5.0 twisted-17.1.0
$ git push
```

When deciding what to rebase, you should generally use
the first commit in the current history that isn't upstream.

## Updating a Project into VTK

Bringing changes into VTK involves first deciding what to bring in. That is specified in the
`update.sh` script under the `tag=` field. Usually this is a `for/vtk` branch, but may
be `master`, or a tag, or any other Git reference.

If `update.sh` needs to be edited (the usual case), create a branch in the usual way
and commit just those changes.

Next, run the `update.sh` script as below. This will update the local copy of the project to
the version specified within.

```sh
$ cd vtk/ThirdParty/zlib
$ git checkout -b update_zlib_YYYY_MM_DD
$ ./update.sh
```

Appending the date to the branch name is not necessary, it just prevents any conflict in the
event of you doing this procedure multiple times and inadvertently using the same branch name.

(All this requires a Git 2.5 or higher due the `worktree` tool being used to
simplify the availability of the commits to the main checkout.)

Make sure to update the `SPDX_DOWNLOAD_LOCATION` in `CMakeLists.txt` to reflect
the changes made to the project.

Now you can review the change and make a merge request from the branch as normal.

## Porting a Project

When converting a project, if there are any local patches, a project should be
created [on Kitware's GitLab](https://gitlab.kitware.com/third-party) to track it
(requests may be filed on the [repo-requests][] repository). If the upstream
project does not use Git, it should be imported into Git (there may be existing
conversions available on Github already). The project's description should
indicate where the source repository lives.

Once a mirror of the project is created, a branch named `for/foo` should be
created where patches for the `foo` project will be applied (i.e., `for/vtk`
for VTK's patches to the project). Usually, changes to the build system, the
source code for mangling, the addition of `.gitattributes` files, and other
changes belong here. Functional changes should be submitted upstream (but may
still be tracked so that they may be used).

For mangling documentation, [some guidelines][] are available.

[repo-requests]: https://gitlab.kitware.com/third-party/repo-requests
[some guidelines]: https://gitlab.kitware.com/third-party/repo-requests/-/wikis/mangling

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

## Process

The basic process involves a second branch where the third party project's
changes are tracked. This branch has a commit for each time it has been
updated and is stripped to only contain the relevant parts (no unit tests,
documentation, etc.). This branch is then merged into the main branch as a
subdirectory using the `subtree` merge strategy.

Initial conversions will require a manual push by the maintainers since the
conversion involves a root commit which is not allowed under normal
circumstances. Please post a message on the [VTK Discourse][] forum asking
for assistance if necessary.

[VTK Discourse]: https://discourse.vtk.org/
