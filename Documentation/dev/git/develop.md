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

1.  Register [GitLab Access].

2.  Follow the [download instructions](download.md#clone) to create a
    local VTK clone:

        $ git clone https://gitlab.kitware.com/vtk/vtk.git VTK
        $ cd VTK

3.  Run the [developer setup script][] to prepare your VTK work tree and
    create Git command aliases used below:

        $ ./Utilities/SetupForDevelopment.sh

4.  (Optional but highly recommended.)
    [Register](https://open.cdash.org/register.php) with the VTK project
    on Kitware's CDash instance to better know how your code performs in
    regression tests.  After registering and signing in, click on
    "All Dashboards" link in the upper left corner, scroll down and click
    "Subscribe to this project" on the right of VTK.

[GitLab Access]: https://gitlab.kitware.com/users/sign_in
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

3.  Edit files and create commits (repeat as needed):

        $ edit file1 file2 file3
        $ git add file1 file2 file3
        $ git commit

    Caveats:
    * To add data follow [these instructions](data.md).
    * If your change modifies the `Utilities/KWSys/vtksys` directory please
      contribute directly to [KWSys][] instead.
    * If your change modifies the `Utilities/MetaIO/vtkmetaio` directory please
      contribute directly to [MetaIO][] instead.

[KWSys]: http://public.kitware.com/Wiki/KWSys/Git
[MetaIO]: https://github.com/Kitware/MetaIO

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

3.  Use the "**Compare branches**" button to proceed to the next page.

4.  Fill out the form and use the "**Submit merge request**" button to
    create the merge request and visit its page.

5.  In the "**Description**" field provide a high-level description
    of the change the topic makes and any relevant information about
    how to try it.  Use `@username` syntax to draw attention of
    specific developers.  If your change is a fix for the `release`
    branch, indicate this so that a maintainer knows it should be
    merged to `release`.

    Optionally use a fenced code block with type `message`, such as

        ```message
        This topic adds a really cool feature.
        ```

    to specify text to be included in the generated merge commit message
    when the topic is [merged](#merge-a-topic).  Do not use `@username`
    syntax in this block as it will not be interpreted.

Review a Merge Request
----------------------

Add comments mentioning specific developers using `@username` syntax to
draw their attention and have the topic reviewed.

### Human Reviews ###

Developers may add comments providing feedback or to acknowledge their
approval.  Reviewers may end their comments with trailing lines of one
of these forms:

    Acked-by: me
    Reported-by: @someone
    Reviewed-by: Some One <someone@somewhere.org>
    Signed-off-by: ...
    Tested-by: ...

These trailers will be extracted during [merging](#merge-a-topic) and
included as trailing lines of the generated merge commit message.
References to `me` and `@username` will automatically be transformed
into a real name and email address according to the user's GitLab
account profile.

### Robot Reviews ###

The "Kitware Robot" automatically performs basic checks on the commits
and adds a comment acknowledging or rejecting the topic.  This will be
repeated automatically whenever the topic is pushed to your fork again.
A re-check may be explicitly requested by adding a comment with the
trailing line:

    Do: check

A topic cannot be [merged](#merge-a-topic) until the automatic review
succeeds.

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
authorized developers may add a comment of the form

    Do: merge

to ask that the change be merged into the upstream repository.

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
