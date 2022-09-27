<!--
This template is for tracking a release of VTK. Please replace the
following strings with the associated values:

  - `@VERSION@` - replace with base version, e.g., 9.1.0
  - `@RC@` - for release candidates, replace with ".rc?". For final, replace with "".
  - `@MAJOR@` - replace with major version number
  - `@MINOR@` - replace with minor version number
  - `@PATCH@` - replace with patch version number
  - `@BASEBRANCH@`: The branch to create the release on (for `x.y.0.rc1`,
    `master`, otherwise `release`)
  - `@BRANCHPOINT@`: The commit where the release should be started

Please remove this comment.
-->

# Update VTK

  - Update the local copy of `@BASEBRANCH@`.
    - If `@PATCH@@RC@` is `0.rc1`, update `master`
    - Otherwise, update `release`
```
git fetch origin
git checkout @BASEBRANCH@
git merge --ff-only origin/@BASEBRANCH@ # if this fails, there are local commits that need to be removed
git submodule update --recursive --init
```
    - If `@BASEBRANCH@` is not `master`, ensure merge requests which should be
      in the release have been merged. The [`backport-mrs.py`][backport-mrs]
      script can be used to find and ensure that merge requests assigned to the
      associated milestone are available on the `release` branch.

  - Make a commit for each of these changes on a single topic (suggested branch
    name: `update-to-v@VERSION@`):
    - Assemble release notes into `Documentation/release/@MAJOR@.@MINOR@.md`.
      - [ ] If `PATCH` is greater than 0, add items to the end of this file.
    - [ ] If `@BASEBRANCH@` is `master`, update the non-patch version in a
          separate commit (so that `master` gets it as well).
    - [ ] Remove old release note files
    - [ ] Update `.gitlab/ci/cdash-groups.json` to track the `release` CDash
          groups
    - [ ] Update `CMake/vtkVersion.cmake` and tag the commit (tag this commit below)
```
$EDITOR CMake/vtkVersion.cmake
git commit -m 'Update version number to @VERSION@@RC@' CMake/vtkVersion.cmake
```
  - Create a merge request targeting `release`
    - [ ] Obtain a GitLab API token for the `kwrobot.release.vtk` user (ask
          @ben.boeckel if you do not have one)
    - [ ] Add the `kwrobot.release.vtk` user to your fork with at least
          `Developer` privileges (so it can open MRs)
    - [ ] Use [the `release-mr`][release-mr] script to open the create the
          Merge Request (see script for usage)
      - Pull the script for each release; it may have been updated since it
        was last used
      - The script outputs the information it will be using to create the
        merge request. Please verify that it is all correct before creating
        the merge request. See usage at the top of the script to provide
        information that is either missing or incorrect (e.g., if its data
        extraction heuristics fail).
    - [ ] Get positive review
    - [ ] `Do: merge`
    - [ ] Push the tag to the main repository
      - [ ] `git tag -a -m 'VTK @VERSION@@RC@' v@VERSION@@RC@ commit-that-updated-vtkVersion.cmake`
      - [ ] `git push origin v@VERSION@@RC@`
  - Gather release assets
    - [ ] Source (from the `build:source` CI job in the tag pipeline)
    - [ ] Documentation (from the `release-prep:documentation` CI job in the tag pipeline)
    - [ ] Wheels (from the `build:wheel-*` jobs).
    - [ ] Wheel SDKs (from the `build:wheel-*` jobs (`vtk-wheel-sdk-*.tar.xz`)).
  - Upload assets to `vtk.org`
    - [ ] `rsync -rptv $tarballs $wheels $wheel_sdks user@host:vtk_release/@MAJOR@.@MINOR@/`
    - [ ] `rsync -rptv $wheel_sdks user@host:wheel-sdks/`
  - [ ] Update `vtk.org/download` with the new release (email
        `comm@kitware.com` with filenames and hashes)
  - Software process updates (these can all be done independently)
    - [ ] Update kwrobot with the new `release` branch rules (@ben.boeckel)
    - [ ] Run [this script][cdash-update-groups] to update the CDash groups
      - This must be done after a nightly run to ensure all builds are in the
        `release` group
      - See the script itself for usage documentation
    - Deprecation updates (if `@BASEBRANCH@` is `master`)
    - [ ] Update deprecation macros for the next release
    - [ ] Remove deprecated symbols from before the *prior* release
    - [ ] Update `VTK_MINIMUM_DEPRECATION_LEVEL` to be that of the *prior*
          release

[backport-mrs]: https://gitlab.kitware.com/utils/release-utils/-/blob/master/backport-mrs.py
[release-mr]: https://gitlab.kitware.com/utils/release-utils/-/blob/master/release-mr.py
[cdash-update-groups]: https://gitlab.kitware.com/utils/cdash-utils/-/blob/master/cdash-update-groups.py

# Post-release

  - [ ] Post an announcement in the Announcements category on
        [discourse.vtk.org](https://discourse.vtk.org/).

/cc @ben.boeckel
/cc @berkgeveci
/cc @vbolea
/milestone %"@VERSION@@RC@"
/label ~"priority:required"
