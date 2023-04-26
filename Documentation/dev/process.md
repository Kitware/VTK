# Software Process

```{note}
Historically, this document was located
[here][https://docs.google.com/document/d/1nzinw-dR5JQRNi_gb8qwLL5PnkGMK2FETlQGLr10tZw).
This has now been archived and the software process is described below.
```

## Introduction

VTK's software process enables decentralized development of the library
and ensures constant software quality. The purpose of this document is
to record and explain the current VTK software development process. By
doing so, we aim to soften the learning curve for developers new and
old.

The process has evolved throughout the lifetime of VTK. By setting down
the process in writing we seek not to hamper that evolution but rather
to provide a defined basis for proposing further changes in order to
continue its improvement. We seek a process that does not place needless
or difficult restrictions on developers, encourages contributions from
new developers, and does not require significant effort to centrally
manage.

## Releases

 We aim for a six-month release cycle. However we allow that this
 schedule is a soft one. The project is funded and developed by many
 different groups, all of which work towards their own particular sets
 of features. In the past, Kitware has released official versions only
 when a customer with a particular need for a release asked for one.

 VTK releases are named with a Major.Minor.Patch scheme. Prior to VTK
 6, odd minor numbers indicated only any place in the span between
 official even minor numbered stable releases. After VTK 6 that scheme
 was dropped.

 The shape of the overall release history is that of a skinny tree.
 Development proceeds along the trunk or `master` branch (taking the
 form of topic branches that start from and are merged into master),
 and every so often a release is tagged and branched from it. In
 general no work goes into the `release` branch, other than the handful
 of important patches that make up the occasional patch release.

 On the master branch, bug fixes and new features are continuously
 developed. At release time, the focus temporarily shifts to producing
 a library that is as stable and robust as possible. The process for
 cutting releases is as follows:

1.  Inform developers that a release is coming

    A few weeks before the intended release branch, announce on
    [VTK Discourse](https://discourse.vtk.org/) that a new release is nearing.
    This alerts developers to hold off making drastic changes that might delay
    the release and gives them a chance to push important and nearly completed
    features in time for the release.

1.  Polish the dashboards and bug tracker

    Persistent compilation and regression test problems are fixed. Serious
    outstanding bugs are fixed.

1.  Forward release branch

    When the dashboards are clean and the outstanding features are
finished, we pick a point on the development branch to be the start of
the next release branch. Next we move the release branch forward from
its current position to the new one.

1.  Do the release candidate cycle

    1.  Tag the release branch and make and publish release candidate
        tar balls and change summaries.

    2.  Announce the release candidate and request feedback from the
        community and especially third party packagers.

        * Bug reports should be entered into the bug tracker with milestone set to the upcoming release number.

    3. If the community does report bugs, the manager classifies them
       in the bug tracker and sees that they are fixed.

       Only serious bugs and regressions need to be fixed before the release.
       New features and minor problems should go into the master branch as
       usual.

       Patches for the release branch must start from the release branch, be
       submitted through gitlab, and merged into master. Once fully tested
       there the branch can be merged into the release branch.

       When the selected issues are fixed in the release branch, the tip of
       the release branch is tagged and released as the next candidate and
       the cycle continues.

1.  Package the official release

    The official VTK package consists of tar balls and zip files of: the
    source, the doxygen documentation, and regression test data, all at
    the tag point. Volunteer third party packagers create binary packages
    from the official release for various platforms, so their input is
    especially valuable during the release cycle.

    The release manager also compiles release notes that go into the
    official release announcement.
    Release notes for release `X.Y` are compiled from topic documents
    added to the `Documentation/release/dev` folder while contributing
    features or fixing issues. The aggregation of the topic files is done
    manually and results in the creation of file named `Documentation/release/X.Y.md`.

    The detailed release process is described in an [issue template](https://gitlab.kitware.com/vtk/vtk/-/blob/master/.gitlab/issue_templates/new-release.md)
