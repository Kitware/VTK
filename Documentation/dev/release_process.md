# Release Process

This document provides a high-level overview of the VTK release cycle and
associated release process.

## Overview

We aim to release a new version of VTK every six months. However, we recognize
that this schedule is flexible. The project is funded and developed by many
different groups, each of which works towards their own particular sets of
features.

VTK releases are named with a `Major.Minor.Patch` scheme.

## Branching Scheme

The overall release history resembles a skinny tree. Development proceeds along
the `master` branch, consisting of topic branches that start from and are merged
into `master`. Every so often, a release is tagged and branched from it.

In general, no work takes place on the `release` branch, other than the handful
of important patches that make up occasional patch releases.

:::{hint}
Steps for contributing changes specific to the `release` branch are documented in
[](git/develop.md#create-a-topic).
:::

On the `master` branch, bug fixes and new features are continuously
developed. At release time, the focus temporarily shifts to producing
a library that is as stable and robust as possible.

## Steps

The process for cutting releases is as follows:

1. Announce upcoming release

   A few weeks before the intended `release` branch, announce on [VTK Discourse](https://discourse.vtk.org/)
   that a new release is coming. This alerts developers to avoid making drastic
   changes that might delay the release and gives them a chance to push important
   and nearly completed features in time for the release. For example,
   see [this post](https://discourse.vtk.org/t/vtk-9-2-0-release-cycle/8149).

2. Polish the dashboards and bug tracker by addressing outstanding issue and
   coordinate effort with relevant developers.

   Persistent compilation and regression test problems are fixed. Serious
   outstanding bugs are fixed.

3. [Create a new issue](https://gitlab.kitware.com/vtk/vtk/-/issues/new) titled
   `Release X.Y.Z[rcN]` based of the [new-release](https://gitlab.kitware.com/vtk/vtk/-/blob/master/.gitlab/issue_templates/new-release.md?plain=1) template.

   :::{Important}
   Specific steps to create eiter the candidate or the official release are found
   in the newly created issue.
   :::

4. Perform the release candidate cycle

    1. Tag the release branch and create and publish release candidate
       artifacts and change summaries.

    2. Announce the release candidate and request feedback from the
       community, especially third-party packagers.

       :::{hint}
       Bug reports should be entered into the bug tracker with the upcoming
       release number as the milestone.
       :::

    3. If the community reports bugs, classify them in the bug tracker and ensure
       they are fixed.

       Only serious bugs and regressions need to be fixed before the release.
       New features and minor problems should be merged into `master` as usual.

       Patches for the release branch should start from the release branch, be
       submitted through GitLab, and then merged into `master`. Once fully
       tested there, the branch can be merged into the release branch.

       When the selected issues are fixed in the release branch, tag the tip
       of the release branch and release it as the next candidate, then the
       cycle continues.

5. Package the official release

   The official VTK package consists of tar balls and ZIP files of the source,
   Python Wheels, Doxygen documentation, and regression test data, all at the
   tag point.

   Volunteer third-party packagers create binary packages from the official
   release for various platforms, so their input is especially valuable during
   the release cycle.

   The release manager also compiles release notes for the official release
   announcement. Release notes are compiled from various [standardized topic documents](https://gitlab.kitware.com/vtk/vtk/-/tree/master/Documentation/release)
   added to the `Documentation/release/dev` folder while features or issues
   are fixed. The aggregation of these topic files is done manually and
   results in the creation of a file named `Documentation/release/X.Y.md` for
   the current release.
