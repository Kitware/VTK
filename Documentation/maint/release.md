# Releasing VTK

Releasing VTK has lots of steps. This document should be a central location for
all of the tribal knowledge around it.

## Current Maintainers

  - Ben Boeckel (@ben.boeckel) <ben.boeckel@kitware.com>
  - Dave DeMarle (@demarle) <dave.demarle@kitware.com>
  - Chuck Atkins (@chuck.atkins) <chuck.atkins@kitware.com>

## Initial steps

Check the bugtracker for critical bugs and close out old items (six months to
a year old) with a note that reopening the bug if it still exists is the next
step.

### GitLab

Create the relevant milestones in GitLab (one for the actual release and
another for rc1). Plausible deadlines should be used; they can be edited later.

### Annoucements

Announcements should be sent to developers first and Linux distribution
maintainers.

#### Mailing List

Announce to the developer list (vtk-developers@vtk.org) that a release is being
planned. Template:

    Hi,

    Does anyone have work in progress that should delay the branch point for
    $version?

    If so, please add the $version_rc1 milestone on any relevant merge requests
    and @mention $maintainers so they can be more easily tracked.

    We are hoping to get $version started by $date.

    Thanks,

    The VTK Maintenance Team

#### Distributions

Patches can accumulate over time in distributions as time goes on. An email
asking if anything needs to go into the next release should be sent to
maintainers of the packages. Ben has a list of maintainers to email.

Here are some places to look for patches:

  - Debian
    * https://anonscm.debian.org/cgit/debian-science/packages/vtk6.git/tree/debian/patches
  - Fedora
    * https://pkgs.fedoraproject.org/cgit/vtk.git/tree
  - Gentoo
    * https://sources.gentoo.org/cgi-bin/viewvc.cgi/gentoo-x86/sci-libs/vtk/files/
  - openSUSE
    * https://build.opensuse.org/package/show/openSUSE:Factory/vtk

### Creating the Branch

Pick a viable first-parent commit from master and run:

```sh
$ git checkout $release_root
$ git checkout -B release
$ git push origin release
```

Email `brad.king@kitware.com` and `ben.boeckel@kitware.com` with the
`release_root` hash and version number so that kwrobot may be updated to check
if merge requests are eligible for this release branch.

Announce to the `vtk-developers@vtk.org` list that the release branch has been
created.

### Release Notes Emails

In `Utilities/Maintenance/release`, there are the following scripts:

  - `make-changelog.sh`
  - `prep-emails.py`
  - `send-emails.sh`

First, create the `changes.txt` file by giving a commit range to
`make-changelog.sh`:

```sh
$ make-changelog.sh $previous_release $release
```

This file contains all of the changes in the commit range grouped by author.
It is then processed by `prep-emails.py`:

```sh
$ ./prep-emails.py $version
```

It takes `changes.txt` and creates a file for each email address:
`user@domain.tld.txt`. You should delete any files which point to upstream or
the developer list which are used to preserve authorship for `ThirdParty`
updates or wide, sweeping changes.

Once that is complete, run `send-emails.sh`:

```sh
$ mailer=mutt ./send-emails.sh $version
```

The `mailer` environment variable should support this command line:

```sh
$ $mailer -s "$subject" $cc_list "$to" < "$body"
```

which is any `sendmail`-compatible program.

## Per-rc Steps

The first release candidate is the initial branch point, so it does not have
special steps to create. However, as `master` moves fairly quickly, branches
need to be corralled into the `release` branch afterwards.

### GitLab Workflow

Once rcN is tagged, create rcN+1. A two week gap should be sufficient as an
initial deadline.

Steps for managing branches on GitLab for the release are documented
[here](gitlab.md).

### Merging Branches

Merging into the `release` branch is done by:

```sh
$ git remote add gl/$user https://gitlab.kitware.com/$user/vtk.git
$ git fetch -p gl/$user
$ git checkout $release_branch
$ git merge --no-ff gl/$user/$branch
$ git push origin $release_branch
```

If the release branch is still being merged into `master`:

```sh
$ git checkout master
$ git merge --no-ff --no-log $release_branch
$ git push origin master
```

## Creating Tarballs

Tarballs need to be created and uploaded for each release. The source tarballs
should be generated in both `.tar.gz` format and `.zip` format. The `.zip`
files are for Windows users and the non-data files contain Windows newline
endings.

### Source

#### Unix

Run (where `$version` is, e.g., 7.0.0.rc2):

```sh
$ Utilities/Maintenance/SourceTarball.bash --tgz -v $version v$version
```

This will generate tarballs for the source and testing data.

#### Windows

From a `git bash` shell with `wget` in `PATH`, run (where `$version` is, e.g.,
7.0.0.rc2):

```sh
$ Utilities/Maintenance/SourceTarball.bash --zip -v $version v$version
```

This should be done on Windows so that the sources have Windows-style newline
endings.

### Documentation

On a machine with `doxygen` installed, configure a build with
`BUILD_DOCUMENTATION=ON` and run the `DoxygenDoc` target. To create the
documentation tarball, run:

```sh
$ tar -C Utilities/Doxygen/doc -czf vtkDocHtml-$version.tar.gz html
```

from the top of the build tree.

### Binaries

The VTK superbuild project should first be updated to build the release.
Update the `versions.cmake` and `CMake/vtk_version.cmake` files. Make a merge
request to the vtk-superbuild project's `master` branch.

To build VTK, set `ENABLE_vtk=ON`, `CMAKE_BUILD_TYPE=Release`,
`ENABLE_python=ON`, and `USE_VTK_MASTER=OFF`. Additionally, set `BUILD_VTK7`
to `ON` or `OFF` depending on the version being built.

If using one of the existing build trees, the build tree should be cleared if
it was not used for a release candidate of the same version being built (e.g.,
if building 7.0.x, ensure that the install tree either doesn't exist or only
has libraries ending in `-vtk7.0`).

#### Linux

##### VTK 6

Find the VM which is used to build. Ben has it archived if a new instance is
needed (`vtkpython_maker`). The superbuild is in
`$HOME/Desktop/vtkbuild/VTK-superbuild` and `$HOME/Desktop/vtkbuild/build`.
Update the superbuild source and run `make` in the build tree. To generate the
binary, run `cpack -G TGZ`.

##### VTK 7

Find the VM which is used to build. Ben has it archived if a new instance is
needed (`vtk7-release`). The superbuild is in `$HOME/code/vtk/src-sb` and
`$HOME/code/vtk/build-sb-release`. Update the superbuild source and run `make`
in the build tree. To generate the binary, run `cpack -G TGZ`.

#### OS X

##### VTK 6

On `karego-at` (in Dave DeMarle's office), reboot into the `10.6-dev`
installation. The superbuild is in `$HOME/Source/VTKSB` and
`$HOME/Source/buildVTKSB`. Update the superbuild source as necessary and run
`make` in the build tree. To generate the binary, run `cpack -G DragNDrop`.

##### VTK 7

SSH into `bigmac`. The superbuild is in `$HOME/code/vtk/src-sb` and
`$HOME/code/vtk/build-sb-release`. Update the superbuild source as necessary
and run `make` in the build tree. To generate the binary, run `cpack -G
DragNDrop`. The deployment target and SDK should be for 10.7.

#### Windows

Also pass `ENABLE_python=ON` and `USE_SYSTEM_python=OFF` to the superbuild.

##### VTK 6

VNC into `miranda`. The source lives in `%USERPROFILE%/DeMarle/VTKSB`. Use the
Visual Studio 2008 command shells (in the Start menu) to compile from
`%USERPROFILE%/DeMarle/build` and `%USERPROFILE%/DeMarle/build32`. Run `cmake
--build . --config Release`. To generate the binary, run `cpack -G NSIS`.

##### VTK 7

VNC into `nemesis` (hostname `nemesis-win64`). The source lives in
`%USERPROFILE%/code/depot/group-kitware/vtk/src-sb`. Use the Visual Studio
2013 command shells (in the Start menu) to compile from
`%USERPROFILE%/code/depot/group-kitware/vtk/build-sb-release`. Use `ninja` to
build the tree. To generate the binary, run `cpack -G NSIS`.

#### Verifying the binaries

On a clean (not setup for development) machine, unpack/install the binaries and
run the vtkpython executable. The version number reported should be correct and
`import vtk; rv=vtk.vtkRenderView(); rv.Render()`
should create a window on the desktop.

#### Tagging the release

When tagging the release, use:

```sh
$ git tag -s -m "VTK $version" v$version $commit_to_be_tagged
```

### Uploading

Upload the files to vtk.org using a public key with access to `vtk.org/files`.
Ask sysadmin to add a key for you. Make sure this is a *new* SSH key and not a
day-to-day use one. In `.ssh/config`:

```conf
Host vtk.release
    User         kitware
    HostName     public.kitware.com
    IdentityFile ~/.ssh/vtk-release
```

and then upload with (where `$release` is analogous to `6.3` or `7.0`):

```sh
$ chmod -R o+r vtk-$release$suffix/
$ rsync -rptv vtk-$release$suffix/ vtk.release:$release
```

### Updating the Website

The website is managed by Wordpress. Access is currently granted to Dave and
Communications:

  - add or modify a `download the latest release candidate` table on the
    [download page](http://www.vtk.org/download/); and
  - update the [documentation page](http://www.vtk.org/documentation) with a
    link to the Doxygen files for the new release.

### Updating the Wiki

The wiki hosts a page which lists API changes between two versions. The
`$workdir` variable should be an *empty* directory, `$srcdir` is usually `.`,
and the release variables are the tags to diff.

```sh
Utilities/Maintenance/semanticDiffVersion.py -w -t $workdir $srcdir $old_release $release > output.wiki
```

The content output by this script should be uploaded to a page on the wiki.
Previous examples:

    http://www.paraview.org/Wiki/VTK/API_Changes_6_1_0_to_6_2_0
    http://www.paraview.org/Wiki/VTK/API_Changes_6_2_0_to_6_3_0

## Announcing

Sen and email to `vtk-developers@vtk.org`, `vtkusers@vtk.org`. Also inform
`comm@kitware.com`.

For the final release, a blog post, release notes, and a Source article should
be made.
