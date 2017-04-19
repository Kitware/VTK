Develop VTK with Git
====================

This page documents how to develop VTK through [Git][].
See the [README](README.md) for more information.

[Git]: http://git-scm.com

Git is an extremely powerful version control tool that supports many
different "workflows" for individual development and collaboration.
Here we document procedures used by the VTK development community.
In the interest of simplicity and brevity we do *not* provide an
explanation of why we use this approach.

Setup
-----

Before you begin, perform initial setup:

1.  Register [GitLab Access] to create an account and select a user name.

2.  [Fork VTK][] into your user's namespace on GitLab.

3.  Follow the [download instructions](download.md#clone) to create a
    local clone of the main VTK repository:

        $ git clone https://gitlab.kitware.com/vtk/vtk.git VTK
        $ cd VTK
    The main repository will be configured as your `origin` remote.

4.  Run the [developer setup script][] to prepare your VTK work tree and
    create Git command aliases used below:

        $ ./Utilities/SetupForDevelopment.sh
    This will prompt for your GitLab user name and configure a remote
    called `gitlab` to refer to it.

5.  (Optional but highly recommended.)
    [Register](https://open.cdash.org/register.php) with the VTK project
    on Kitware's CDash instance to better know how your code performs in
    regression tests.  After registering and signing in, click on
    "All Dashboards" link in the upper left corner, scroll down and click
    "Subscribe to this project" on the right of VTK.

[GitLab Access]: https://gitlab.kitware.com/users/sign_in
[Fork VTK]: https://gitlab.kitware.com/vtk/vtk/forks/new
[developer setup script]: /Utilities/SetupForDevelopment.sh

Workflow
--------

VTK development uses a [branchy workflow][] based on topic branches.
Our collaboration workflow consists of three main steps:

1.  Local Development:
    * [Update](#update)
    * [Create a Topic](#create-a-topic)

2.  Code Review (requires [GitLab Access][]):
    * [Share a Topic](#share-a-topic)
    * [Create a Merge Request](#create-a-merge-request)
    * [Review a Merge Request](#review-a-merge-request)
    * [Revise a Topic](#revise-a-topic)

3.  Integrate Changes:
    * [Merge a Topic](#merge-a-topic) (requires permission in GitLab)
    * [Delete a Topic](#delete-a-topic)

[branchy workflow]: http://public.kitware.com/Wiki/Git/Workflow/Topic

Update
------

1.  Update your local `master` branch:

        $ git checkout master
        $ git pull

2.  Optionally push `master` to your fork in GitLab:

        $ git push gitlab master
    to keep it in sync.  The `git gitlab-push` script used to
    [Share a Topic](#share-a-topic) below will also do this.

Create a Topic
--------------

All new work must be committed on topic branches.
Name topics like you might name functions: concise but precise.
A reader should have a general idea of the feature or fix to be developed given just the branch name.

1.  To start a new topic branch:

        $ git fetch origin

2.  For new development, start the topic from `origin/master`:

        $ git checkout -b my-topic origin/master

    For release branch fixes, start the topic from `origin/release`, and
    by convention use a topic name starting in `release-`:

        $ git checkout -b release-my-topic origin/release

    If backporting a change, you may rebase the branch back onto
    `origin/release`:

        $ git checkout -b release-my-topic my-topic
        $ git rebase --onto origin/release origin/master

    Alternatively, for more targeted or aggregate backports, use the `-x` flag
    when performing `git cherry-pick` so that a reference to the original
    commit is added to the commit message:

        $ git checkout -b release-my-topic origin/release
        $ git cherry-pick -x $hash_a $hash_b $hash_c
        $ git cherry-pick -x $hash_d $hash_e $hash_f

3.  Edit files and create commits (repeat as needed):

        $ edit file1 file2 file3
        $ git add file1 file2 file3
        $ git commit

    Caveats:
    * To add data follow [these instructions](data.md).
    * If your change modifies third party code, see [its
      documentation](../../../ThirdParty/UPDATING.md).

Guidelines for Commit logs
--------------------------

Remember to *motivate & summarize*. When writing commit logs, make sure
that there is enough information there for any developer to read and glean
relevant information such as:

1.  Is this change important and why?
2.  If addressing an issue, which issue(s)?
3.  If a new feature, why is it useful and/or necessary?
4.  Are there background references or documentation?

A short description of what the issue being addressed and how will go a long way
towards making the log more readable and the software more maintainable.

Style guidelines for commit logs are as follows:

1. Separate subject from body with a blank line
2. Limit the subject line to 60 characters
3. Capitalize the subject line
4. Use the imperative mood in the subject line e.g. "Refactor foo" or "Fix Issue #12322",
   instead of "Refactoring foo", or "Fixing issue #12322".
5. Wrap the body at 80 characters
6. Use the body to explain `what` and `why` and if applicable a brief `how`.

Share a Topic
-------------

When a topic is ready for review and possible inclusion, share it by pushing
to a fork of your repository in GitLab.  Be sure you have registered and
signed in for [GitLab Access][] and created your fork by visiting the main
[VTK GitLab][] repository page and using the "Fork" button in the upper right.

[VTK GitLab]: https://gitlab.kitware.com/vtk/vtk

1.  Checkout the topic if it is not your current branch:

        $ git checkout my-topic

2.  Check what commits will be pushed to your fork in GitLab:

        $ git prepush

3.  Push commits in your topic branch to your fork in GitLab:

        $ git gitlab-push

    Notes:
    * If you are revising a previously pushed topic and have rewritten the
      topic history, add `-f` or `--force` to overwrite the destination.
    * If the topic adds data see [this note](data.md#push).
    * The `gitlab-push` script also pushes the `master` branch to your
      fork in GitLab to keep it in sync with the upstream `master`.

    The output will include a link to the topic branch in your fork in GitLab
    and a link to a page for creating a Merge Request.

Create a Merge Request
----------------------

(If you already created a merge request for a given topic and have reached
this step after revising it, skip to the [next step](#review-a-merge-request).)

Visit your fork in GitLab, browse to the "**Merge Requests**" link on the
left, and use the "**New Merge Request**" button in the upper right to
reach the URL printed at the end of the [previous step](#share-a-topic).
It should be of the form:

    https://gitlab.kitware.com/<username>/vtk/merge_requests/new

Follow these steps:

1.  In the "**Source branch**" box select the `<username>/vtk` repository
    and the `my-topic` branch.

2.  In the "**Target branch**" box select the `vtk/vtk` repository and
    the `master` branch.  It should be the default.

    If your change is a fix for the `release` branch, you should still
    select the `master` branch as the target because the change needs
    to end up there too.

    For other `release` branches (e.g., `release-6.3`), merge requests should
    go directly to the branch (they are not tied with `master` in our
    workflow).

3.  Use the "**Compare branches**" button to proceed to the next page
    and fill out the merge request creation form.

4.  In the "**Title**" field provide a one-line summary of the entire
    topic.  This will become the title of the Merge Request.

    Example Merge Request Title:

        Wrapping: Add Java 1.x support

5.  In the "**Description**" field provide a high-level description
    of the change the topic makes and any relevant information about
    how to try it.
    *   Use `@username` syntax to draw attention of specific developers.
        This syntax may be used anywhere outside literal text and code
        blocks.  Or, wait until the [next step](#review-a-merge-request)
        and add comments to draw attention of developers.
    *   If your change is a fix for the `release` branch, indicate this
        so that a maintainer knows it should be merged to `release`.
    *   Optionally use a fenced code block with type `message` to specify
        text to be included in the generated merge commit message when the
        topic is [merged](#merge-a-topic).

    Example Merge Request Description:

        This branch requires Java 1.x which is not generally available yet.
        Get Java 1.x from ... in order to try these changes.

        ```message
        Add support for Java 1.x to the wrapping infrastructure.
        ```

        Cc: @user1 @user2

6.  The "**Assign to**", "**Milestone**", and "**Labels**" fields
    may be left blank.

7.  Use the "**Submit merge request**" button to create the merge request
    and visit its page.

Guidelines for Merge Requests
-----------------------------

Remember to *motivate & summarize*. When creating a merge request, consider the
reviewers and future perusers of the software. Provide enough information to motivate
the merge request such as:

1.  Is this merge request important and why?
2.  If addressing an issue, which issue(s)?
3.  If a new feature, why is it useful and/or necessary?
4.  Are there background references or documentation?

Also provide a summary statement expressing what you did and if there is a choice
in implementation or design pattern, the rationale for choosing a certain path.
Notable software or data features should be mentioned as well.

A well written merge request will motivate your reviewers, and bring them up
to speed faster. Future software developers will be able to understand the
reasons why something was done, and possibly avoid chasing down dead ends,
Although it may take you a little more time to write a good merge request,
you’ll likely see payback in faster reviews and better understood and
maintainable software.


Review a Merge Request
----------------------

Add comments mentioning specific developers using `@username` syntax to
draw their attention and have the topic reviewed.  After typing `@` and
some text, GitLab will offer completions for developers whose real names
or user names match.

Comments use [GitLab Flavored Markdown][] for formatting.  See GitLab
documentation on [Special GitLab References][] to add links to things
like merge requests and commits in other repositories.

[GitLab Flavored Markdown]: https://gitlab.kitware.com/help/markdown/markdown
[Special GitLab References]: https://gitlab.kitware.com/help/markdown/markdown#special-gitlab-references

### Human Reviews ###

Reviewers may add comments providing feedback or to acknowledge their
approval.  Lines of specific forms will be extracted during
[merging](#merge-a-topic) and included as trailing lines of the
generated merge commit message.

A commit message consists of up to three parts which must be specified
in the following order: the [leading line](#leading-line), then
[middle lines](#middle-lines), then [trailing lines](#trailing-lines).
Each part is optional, but they must be specified in this order.

#### Leading Line ####

The *leading* line of a comment may optionally be exactly one of the
following votes followed by nothing but whitespace before the end
of the line:

*   `-1` or :-1: (`:-1:`) means "The change is not ready for integration."
*   `+1` or :+1: (`:+1:`) means "I like the change but defer to others."
*   `+2` means "The change is ready for integration."
*   `+3` means "I have tested the change and verified it works."

#### Middle Lines ####

The middle lines of a comment may be free-form [GitLab Flavored Markdown][].

#### Trailing Lines ####

Zero or more *trailing* lines in the last section of a comment may
each contain exactly one of the following votes followed by nothing
but whitespace before the end of the line:

*   `Rejected-by: me` means "The change is not ready for integration."
*   `Acked-by: me` means "I like the change but defer to others."
*   `Reviewed-by: me` means "The change is ready for integration."
*   `Tested-by: me` means "I have tested the change and verified it works."

Each `me` reference may instead be an `@username` reference or a full
`Real Name <user@domain>` reference to credit someone else for performing
the review.  References to `me` and `@username` will automatically be
transformed into a real name and email address according to the user's
GitLab account profile.

#### Fetching Changes ####

One may fetch the changes associated with a merge request by using
the `git fetch` command line shown at the top of the Merge Request
page.  It is of the form:

    $ git fetch https://gitlab.kitware.com/$username/vtk.git $branch

This updates the local `FETCH_HEAD` to refer to the branch.

There are a few options for checking out the changes in a work tree:

*   One may checkout the branch:

        $ git checkout FETCH_HEAD -b $branch
    or checkout the commit without creating a local branch:

        $ git checkout FETCH_HEAD

*   Or, one may cherry-pick the commits to minimize rebuild time:

        $ git cherry-pick ..FETCH_HEAD

### Robot Reviews ###

The "Kitware Robot" automatically performs basic checks on the commits
and adds a comment acknowledging or rejecting the topic.  This will be
repeated automatically whenever the topic is pushed to your fork again.
A re-check may be explicitly requested by adding a comment with a single
[*trailing* line](#trailing-lines):

    Do: check

A topic cannot be [merged](#merge-a-topic) until the automatic review
succeeds.

### Testing ###

VTK has a [buildbot](http://buildbot.net) instance watching for merge requests
to test.  A developer must issue a command to buildbot to enable builds:

    Do: test

The buildbot user (@buildbot) will respond with a comment linking to the CDash
results when it schedules builds.

The `Do: test` command accepts the following arguments:

  * `--oneshot`
        only build the *current* hash of the branch; updates will not be built
        using this command
  * `--stop`
        clear the list of commands for the merge request
  * `--superbuild`
        build the superbuilds related to the project
  * `--clear`
        clear previous commands before adding this command
  * `--regex-include <arg>` or `-i <arg>`
        only build on builders matching `<arg>` (a Python regular expression)
  * `--regex-exclude <arg>` or `-e <arg>`
        excludes builds on builders matching `<arg>` (a Python regular
        expression)

Multiple `Do: test` commands may be given in separate comments. A new `Do: test`
command must be explicitly issued for each branch update for which testing is
desired. Buildbot may skip tests for older branch updates that have not started
before a test for a new update is requested.

Builder names always follow this pattern:

        project-host-os-libtype-buildtype+feature1+feature2

  * project: always `vtk` for vtk
  * host: the buildbot host
  * os: one of `windows`, `osx`, or `linux`
  * libtype: `shared` or `static`
  * buildtype: `release` or `debug`
  * feature: alphabetical list of features enabled for the build

For a list of all builders, see:

  * [vtk-expected](https://buildbot.kitware.com/builders?category=vtk-expected)
  * [vtk-superbuild](https://buildbot.kitware.com/builders?category=vtk-superbuild)
  * [vtk-experimental](https://buildbot.kitware.com/builders?category=vtk-experimental)

Revise a Topic
--------------

If a topic is approved during GitLab review, skip to the
[next step](#merge-a-topic).  Otherwise, revise the topic
and push it back to GitLab for another review as follows:

1.  Checkout the topic if it is not your current branch:

        $ git checkout my-topic

2.  To revise the `3`rd commit back on the topic:

        $ git rebase -i HEAD~3

    (Substitute the correct number of commits back, as low as `1`.)
    Follow Git's interactive instructions.

3.  Return to the [above step](#share-a-topic) to share the revised topic.

Merge a Topic
-------------

After a topic has been reviewed and approved in a GitLab Merge Request,
authorized developers may add a comment with a single
[*trailing* line](#trailing-lines):

    Do: merge

to ask that the change be merged into the upstream repository.  By
convention, do not request a merge if any `-1` or `Rejected-by:`
review comments have not been resolved and superseded by at least
`+1` or `Acked-by:` review comments from the same user.

### Merge Success ###

If the merge succeeds the topic will appear in the upstream repository
`master` branch and the Merge Request will be closed automatically.

### Merge Failure ###

If the merge fails (likely due to a conflict), a comment will be added
describing the failure.  In the case of a conflict, fetch the latest
upstream history and rebase on it:

    $ git fetch origin
    $ git rebase origin/master

(If you are fixing a bug in the latest release then substitute
`origin/release` for `origin/master`.)

Return to the [above step](#share-a-topic) to share the revised topic.

Delete a Topic
--------------

After a topic has been merged upstream the Merge Request will be closed.
Now you may delete your copies of the branch.

1.  In the GitLab Merge Request page a "**Remove Source Branch**"
    button will appear.  Use it to delete the `my-topic` branch
    from your fork in GitLab.

2.  In your work tree checkout and update the `master` branch:

        $ git checkout master
        $ git pull

3.  Delete the local topic branch:

        $ git branch -d my-topic

    The `branch -d` command works only when the topic branch has been
    correctly merged.  Use `-D` instead of `-d` to force the deletion
    of an unmerged topic branch (warning - you could lose commits).
