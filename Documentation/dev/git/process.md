# Software Process

```{note}
Historically, this document was located
[here][https://docs.google.com/document/d/1nzinw-dR5JQRNi_gb8qwLL5PnkGMK2FETlQGLr10tZw).
This has now been archived and the software process is described below.
```

```{warning}
This needs to be updated to reflect the current process, 2023.04.24
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

    A few weeks before the intended release branch, announce on the
mailing list that a new release is nearing. This alerts developers to
hold off making drastic changes that might delay the release and gives
them a chance to push important and nearly completed features in time
for the release.

1.  Polish the dashboards and bug tracker

    Persistent compilation and regression test problems are fixed. Serious
    outstanding bugs are fixed.

1.  Forward release branch

    When the dashboards are clean and the outstanding features are
finished, we pick a point on the development branch to be the start of
the next release branch. Next we move the release branch forward from
its current position to the new one.

1.  Gather descriptions of changes

    1.  Compile a list of developers and their changes and send emails
        to each developer asking them to summarize their work.

    1.  Run the API differencer script in
        `Utilities/Maintainance/semanticDiffVersion.py`

1.  Do the release candidate cycle

    1.  Tag the release branch and make and publish release candidate
        tar balls and change summaries.

    2.  Announce the release candidate and request feedback from the
        community and especially third party packagers.

        * Bug reports should be entered into the bug tracker with milestone set to the upcoming release number.
        * If no important bugs are reported fourteen days after the
    candidate is published, the source is re-tagged and packaged as the
    official release.

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



## Contributing Code

VTK's development history is stored in a git server running at Kitware, and mirrored elsewhere. We maintain two long-lived branches, master and release. Only the release manager has permissions to push changes onto to the release branch. On the master branch, all changes go through Kitware's Gitlab  source code review system instance. Gitlab is a full git hosting instance with a web interface through which developers can discuss proposed changes. Kitware's Gitlab instance is tied to buildbot regression testing servers that compile and run VTK's regression tests on each change from trusted developers.  The core group of trusted developers also has permission to merge new code from Gitlab onto the master branch. Developers not in the core group can push their changes to the Gitlab server to share it with the the community who can review, test and merge the change.

All code is contributed through Gitlab as doing so automates and thus greatly reduces the time and effort needed to validate each change. Merge requestscan request buildbot builds and regression tests on Windows, Mac, and Linux volunteer machines. Code contributed by others is tested whenever someone in the core group adds a review comment that says "Do: test".  After the tests complete, anyone in the core group can merge the change into the master branch by positively reviewing the code and commenting "Do: merge".

Step by step instructions on how to commit changes to VTK through Gitlab are maintained as part of the VTK source code: <https://gitlab.kitware.com/vtk/vtk/blob/master/Documentation/dev/git/develop.md>. Tips for using Git and Gitlab include:

1.  To determine who you should ask to review your code, type "git log ---follow" in your source tree. The most recent developers to work on a file you change are usually the best reviewers.

1.  To share a change that you are not finished with, and do not want merged into master, start your commit message with "WIP:" for work in progress. Alternatively you can give yourself a -2 review score.

1.  If you are a core developer and want to test an externally contributed change, make a comment of "Do: test " on the MR.

1.  To merge a change, make a comment of "Do: merge " on the MR.

1.  Self review of code is not allowed. Only rarely should you merge your own topics before another developer gives you a +2 score. Repeat offences will result in removal of your merge privileges.

1.  Make sure the author knows when their work is merged into master so that they know when to pay special attention to the continuous and nightly dashboards. For work from external developers, the developer who did the merge should send an email and help look for problems. Core developers should not merge each other's changes.

1.  Topics should not be overly long nor entirely compressed. It is best to consolidate your work into a handful of substantial changes that are functional on their own. This helps others understand and review your work and eases the task of keeping it up to date with master. At the same time it is not good practice to habitually squash topics into single commits, as this obscures too much of the thinking that went into the final result.

1.  [Gitlab offers multiple ways to associate Issues and Merge Requests](https://about.gitlab.com/2016/03/08/gitlab-tutorial-its-all-connected/).

After a change is merged into the master branch, the changes are further tested as described in the Regression Testing section below. It is important that every developer watches the dashboard, particularly immediately following and on the morning after their topics are merged, to ensure that the code works well on all of the platforms that VTK supports.

It is recommended that the VTK Architecture Review Board (ARB) is consulted before major changes impacting backwards compatibility are developed. The ARB is a consortium of interested parties that help to direct the evolution of VTK. Details can be found at: <http://www.vtk.org/Wiki/VTK/ARB>. If you are unsure if your change should be classified as substantial, it is a good idea to send an email to the VTK developers list and explain what you intend to do.

It is especially important that substantial backwards incompatible changes are discussed with and agreed upon by the community. Minor backwards incompatible changes are allowed in general with the following deprecation policy. Code that is to be removed is marked using the VTK_LEGACY macros in Common/Core/vtkSetGet.h and with the use of @deprecated Doxygen comments in the header files. Deprecated code can then be removed after a minimum of one minor release, i.e. deprecated and released in 6.1, can be removed after 6.1 for the 6.2 release. Backward incompatible changes should be kept to a minimum, we aim for source but not ABI compatibility between minor releases.


## Regression Testing

###  Testing and dashboard submitter setup

Regression testing in VTK takes the form of a set of programs, that are included  in the VTK source code and enabled in builds configured through CMake to have the BUILD_TESTING flag turned on. Test pass/fail results are returned to CTest via a test program's exit code. VTK contains helper classes that do specific checks, such as comparing a produced image against a known valid one, that are used in many of the regression tests.  Test results may be submitted to Kitware's CDash instance, were they will be gathered and displayed at <http://open.cdash.org/index.php?project=VTK.>

All proposed changes to VTK are automatically tested on Windows, Mac and Linux machines as described in the Contributing Code section above. All changes that are merged into the master branch are subsequently tested again by more rigorously configured Windows, Mac and Linux continuous dashboard submitters. After 9PM Eastern Time, the master branch is again tested by a wider set of machines and platforms. These results appear in the next day's page.

At each step in the code integration path the developers who contribute and merge code are responsible for checking the test results to look for problems that the new code might have introduced. Plus signs in CDash indicate newly detected  problems. Developers can correlate problems with contributions by logging in to CDash. Submissions that contain a logged in developer's change are highlighted with yellow dots.

It is highly recommended that developers test changes locally before submitting them. To run tests locally:

1.  Configure with BUILD_TESTING set ON

    The exact set of tests created depends on many configuration options. Tests in non-default modules are only tested when those modules are purposefully enabled, the smoke tests described in the Coding Style section above are enabled only when the python or Tcl interpreter is installed, tests written in wrapped languages are only enabled when wrapping is turned on, etc.

1.  Build. 

    VTK tests are only available from the build tree.

1.  Run ctest at the command line in the build directory or make the TESTING target in Visual Studio.

    As ctest runs the tests it prints a summary. You should expect 90% of the tests or better to pass if your VTK is configured correctly. Detailed results (which are also printed if you supply a --V argument to ctest) are put into the Testing/Temporary directory. The detailed results include the command line that ctest uses to spawn each test. Other particularly useful arguments are:
    ```bash
    --R TestNameSubstringToInclude to choose tests by name

    --E TestNameSubstringToExclude to reject tests by name

    --I start,stop,step to run a portion of the tests

    --j N to run N tests simultaneously.
    ```

Dashboard submitting machines work at a slightly higher level of abstraction that adds the additional stages of downloading, configuring and building VTK before running the tests, and submitting all results to CDash afterward. With a build tree in place you can run "ctest --D Experimental"  to run at this level and submit the results to the experimental section of the VTK dashboard or "ctest --M Experimental -T Build --T Submit" etc to pick and choose from among the stages. When setting up a test submitter machine one should start with the experimental configuration and then, once the kinks are worked out, promote the submitter to the Nightly section.

The volunteer machines use cron or Windows task scheduler to run CMake scripts that configure a VTK build with specific options, and then run ctest --D as above. Within CDash, you can see each test machine's specific configuration by clicking on the Advanced View and then clicking on the note icon in the Build Name column. This is a useful starting point when setting up a new submitter. It is important that each submitter's dashboard script include the name of the person who configures or maintains the machine so that, when the machine has problems, the dashboard maintainer can address it.

At Kitware, the dashboard scripts are maintained in a separate git repository and generally include a common script. The common script facilitates the task of setting up the submitter. See <http://www.vtk.org/Wiki/VTK/Git/Dashboard> for more information.


### Interpreting the Dashboard Results

The dashboard presents all submitted results, for the present day, in a tabular format. Kitware's CDash server keeps the last four months worth of results which you can browse through via the web interface. On any given day, rows in the table are results for one particular submission from one particular submitting machine. Columns in the table show each machine's identity and results for the download, configure, build and test stages. Good results show as green, bad results show as red and in between results are shown in orange. (In the next CDash release the color scheme will be configurable in the browser.) On any result you can click through to drill down and get more detail.

 **NOTE The following are out of date. We need to take out references to removed sections and add the new buildbot CI ones.**

The rows in the table are organized into sections. These are:

-   Nightly Expected

    Nightly results from reliable and trusted machines. All developers pay close attention to this section to see that their changes work on an assortment of platforms and configurations.

-   Continuous

    Daily results that test newly merged changes into the master branch. This is an intermediate step between quick gitlab/buildbot tests and the full nightly test gamut.

-   Dependencies

    Daily results from machines that build VTK with development versions of VTK's dependencies: CMake, Mesa, etc. Failures in this section indicate to the developers that VTK may need to change to keep pace with them.

-   Nightly

    Nightly results from unproven machines. If and when persistent problems with a nightly submitter are fixed, and the responsible person agrees to maintain it, the dashboard manager can promote it to the Nightly Expected section so that more developers will pay close attention to it.

-   Merge-Requests

    This is where results from proposed Merge Requests on Gitlab go.  It is generally more useful to view these results directly though the specific Gitlab's Merge Request page.

-   Experimental

    Submissions from new machines and tests of experimental code can go here. This is good place to test code that has not been merged into the mainstream.

-   Release Branch 

    Nightly results that test the release branch, this is mostly of interest to the release manager during the release candidate cycle.

-   Release-X Branch 

    Nightly results that test an older release branch. This is currently being used to ensure that the 5.10 branch remains functional while people adopt 6.x.

-   Coverage

    This section contains results from machines that have been configured to do code coverage counts that count the number of lines in the VTK source that are exercised or missed by the regression test suite.

-   Dynamic Analysis

    This section contains results from machines that have been configured to run the regression tests through a memory checker to expose memory leaks and harder to find code problems.

In the Utilities/Maintenance directory of the VTK source code there are a few helper scripts that present alternative views of the data collected by CDash, and one script that is useful when setting up a new memory checking submission. These are:

-   parse_valgrind.py - eases the task of setting up a memory checking submitter.

-   vtk_fail_summary.py -- presents a test centric view of failures so that problematic tests can be readily identified.

-   vtk_submitter_summary.py -- determines the configuration of each submitter to help determine under-tested platform and feature sets and correlate failures with causes. This is very useful when trying to tease apart the different roles that each of the dashboard submitters fulfill.

-   vtk_site_history.py -- presents a time centric view of failures to gauge project and dashboard health with.

-   computeCodeCoverage*.sh -- presents coverage results in a more intuitive and helpful way than CDash currently does.



### Writing new tests


All new features that go into VTK must be accompanied by tests. This ensures that the feature works on many platforms and that it will continue to work as VTK evolves.

Tests for the classes in each module of VTK are placed underneath the module's Testing/<Language> subdirectory. Modules that the tests depend upon beyond those that the module itself depends upon are declared with the TEST_DEPENDS argument in the module.cmake file. Test executables are added to VTK's build system by naming them in the CMakeLists.txt files in each Testing/<Language> directory. In those CMakeLists, standard add_executable() + add_test() command pairs could be used, but the following macros defined in vtkTestingMacros.cmake are preferable as they consolidate multiple tests together, participate in VTK's modular build scripts, and ensure consistency:

-   vtk_add_test_cxx(name.cxx [NO_DATA] [NO_VALID] [NO_OUTPUT]) 

    adds a test written in C++

    NO_DATA indicates that this test doesn't require any input data files

    NO_VALID indicates that this test doesn't compare results against baseline images

    NO_OUTPUT indicates that the test doesn't produce any output files

-   vtk_add_test_mpi(name [TESTING_DATA])

    adds a test which should be run as an MPI job. 

    TESTING_DATA indicates that this test looks for input data files and produces regression test images.

-   vtk_add_test_python(name [NO_RT] [NO_DATA] [NO_VALID] [NO_OUTPUT])

    adds a test written in python

    NO_RT indicates that the test won't use the image comparison helpers from vtk.test.testing

-   vtk_add_test_tcl(name [NO_RT] [NO_DATA] [NO_VALID] [NO_OUTPUT])

    NO_RT means that your test won't use the image comparison helpers from rtImageTest.tcl

Tests indicate success to CTest by returning EXIT_SUCCESS (0) and failure by returning EXIT_FAILURE (1). How the test determines what result to return is up to the developer. VTK contains a number of utilities for this task. For example, vtkRegressionTester is a helper class that does a fuzzy comparison of images drawn by VTK against known good baseline images and returns a metric that can be simply compared against a numeric threshold.

**(Write and link to a source article that describes them in enough detail so that adding new tests is an entirely rote process.)**

Many tests require data files to run. The image comparison tests for example need baseline images to compare against, and many tests open up one or more files to visualize. In the past, test data was kept in two external repositories. One each for small and large data files. Since VTK 6.1.0, data is placed on a public web server automatically when topics are pushed, and copied to a Midas <http://www.midasplatform.org> instance at release time. The add_test macros mentioned above use CMake to download the version of the data file that exactly matches the version needed by the source code during the build process.

The source code and data file versions are kept in sync because the Testing/Data directory contains, instead of the real files, similarly named files which contain only the MD5 hash of the matching data files. During the build process, when CMake sees that a required data file is not available, it downloads it into the directory defined by the ExternalData_OBJECT_STORES cmake configuration entry. The test executables read all data from there. The default setting for ExternalData_OBJECT_STORES is the ExternalData directory underneath the VTK build tree.

To make a change to VTK that modifies or adds a new test data file, place the new version in the Testing/Data or directory (for input data files) or Module/Name/Testing/Data (for regression test images), and build (or run cmake). CMake will do the work of moving the original out of the way and replacing it with an MD5 link file. When you push the new link file to Gitlab, git pre-commit hooks push the original file up to Kitware's data service, where everyone can retrieve it.\

 Step by step descriptions for how to add new test data are given here:  <http://www.vtk.org/Wiki/VTK/Git/Develop/Data>

### Bug and Feature Tracking

Despite all of this software quality process, people occasionally find bugs in VTK. Bugs should be reported to the developers so that they might be addressed in future releases. Bugs should be reported first on the user's mailing list for discussion. In a technically sophisticated area like visualization, user error is always possible. If people on the list confirm that the problem is in fact a new bug, or if no one on the list responds to the report, then please enter the bug into the bug tracker.

**NOTE: The following is out of date - we now use issues within gitlab **

We track bugs in the mantis bug tracker instance at: <http://vtk.org/Bug/my_view_page.php>.  As always proper netiquette is valued. First search to see if someone else has already reported the bug. Second, realize that there is no guarantee that a reported bug will be corrected promptly or at all. In the event that no one takes interest in your bug know that Kitware offers paid support options that you can find at <http://www.kitware.com/products/support.html>. Third, the most important thing to try to do in the report is to make it easy to reproduce the problem.

Feature requests should not be made through the bug tracker. The proper channels for feature requests include the the mailing list, Kitware's paid support options (refer to <http://www.kitware.com/products/support.html>), and User Voice. User Voice is a feature voting web service that you can find at: <http://vtk.uservoice.com/forums/31508-general>. From time to time, we take the requested features into account when we decide which direction to steer VTK. Feature requests made on the bug tracker are handled by directing the reporter to the above resources.

Patches are likewise discouraged in the bug tracker. The effort necessary to validate and integrate suggested changes is rarely insignificant. Patch requests made via the issue tracker or mailing list are handled by directing the reporter to Gitlab.

When reporting a bug the important things to describe are:

-   Summary: A concise description of the problem. Always try to include searchable terms in the summary so that other reporters may find it later.

-   Description: It is essential to describe the problem in enough detail to reproduce it. Ideally the description should have URLs or attachments that contain concise runnable code samples and any small data files needed to reproduce it. Links to Gitlab reviews and email discussions are extremely helpful. The perfect way to demonstrate a problem is to make the demonstration into new test and submit it via Gitlab. This make it trivial to reproduce and, once merged will ensure that the problem remains fixed.

-   Found in Version (available only once the bug is submitted)

What VTK release was the problem detected in. Versions ending in dev and rc have special meaning. "dev" indicates that the bug was found in the master branch and not on a released version. Ie 6.0.dev means that the bug was found in the master branch sometime after 6.0.0 release. The fix for this might end up in 6.1.0, and possibly in 6.0.1 as well. "rc" indicates that the bug was found during the release candidate testing cycle. The release manager uses it to track problems and related Gitlab merge requests that need to be addressed before the release is finalized. The related Target Version and Fixed in Version fields exist to help plan what should be fixed in upcoming versions and to keep track of what actually was fixed in past versions.

-   OS and platform (available once the bug is submitted)

This is sometimes necessary to reproduce the problem.

Reporters can generally ignore the rest of the fields. The Project and Type fields are largely ignored in VTK but are used in other projects. The Priority and Assign To fields are for developers and the release manager to use while addressing reported issues.

When a new bug report is made, mantis sends it off to the developer's mailing list. Meanwhile, the release manager reads all newly submitted bugs and asks developers who are familiar with the area of VTK that the bug relates to to do a cursory review. These developers may or may not decide to take the bug up. At VTK release time, the release manager reviews all bugs, decides if any of them are critical to fix before the release, and ensures that they are addressed. Bugs that remain in the bug tracker for more than one year will be expired. When bugs are fixed, the fixes happen in the master branch and generally only appear in subsequent minor releases.

The Status field in the bug tracker may be in one of several states, depending on where a bug is along the path to being fixed. The bug states are a subset of those in ParaView's Kanban inspired software process, in which pools of new work are taken up by various developers and pursued until the requesting customer is satisfied with their resolution. Details of the ParaView's software process state machine are given here:  <http://paraview.org/ParaView/index.php/Development_Workflow>

VTK has a more dispersed set of contributors and less well defined developer roles and funding sources. As such we simplify to the following states and transitions.

-   backlog - All bugs start off in this general pool of work.

-   active development - When a developer begins working on the bug he or she should move it into the active development state to indicate that work has begun.

-   gitlab review - When a developer pushes a fix for the problem into gitlab to be reviewed he or she should move it into the gitlab review state to indicate that a fix is ready for testing.

-   closed -  When the code has been fixed, reviewed, merged into master, and the dashboards are green, the developer moves it to the closed state and sets the Fixed in Version to the current 'dev' version. Closed bugs, if found to be broken, can be reopened, in which case they go back to the backlog state.

-   expired -  When the bug has not gathered attention for a long time it moves into the expired state. Expired bugs can be reopened and placed back in the backlog.


## Documentation

The VTK FAQ (<http://www.vtk.org/Wiki/VTK_FAQ>), examples repository (<http://www.vtk.org/Wiki/VTK/Examples>) and overall Wiki  are community efforts (<http://www.vtk.org/Wiki/VTK>) and open to all. Only rarely is there a concerted effort made by the community to keep them up-to-date.

The Examples Wiki is regression nightly by at least one Linux volunteer machine and verifies that the examples work on the most recent VTK releases. The results can be found at:

<http://open.cdash.org/index.php?project=VTKWikiExamples>. Note that fewer developers pay close attention to these results than to VTK's own regression test results, so it is a good idea to warn the mailing list when problems are found.

Kitware updates and releases new versions of the VTK Textbook and User's guide every few years as demand calls for them to be.
