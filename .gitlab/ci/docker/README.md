# Updating CI images

CI images for Linux are stored within this directory. Each subdirectory is a
different image.

## Layout

Each image should use a shell script to perform its actions rather than bare
`RUN` commands in the `Dockerfile`. The rationale is that:

  - commands can be commented
  - caches can easily be cleaned up between image layers
  - the script exists in the image for documentation of how it was built

## Building

No arguments should be required to build these images. Note that if `podman` is
used, the `--format docker` flag is needed to create compatible images.

## Tagging scheme

CI images should be tagged as:

    kitware/vtk:ci-DIRECTORY-YYYYMMDD

where `DIRECTORY` is the name of the directory and `YYYYMMDD` is the date the
image is built. If multiple images are committed to `master` a day, use an
additional `.N` suffix where `N` is a sequential integer.

## Pushing images

Pushing images requires a token in order to push to the Kitware repository.
Please send an email to Brad King requesting a token.

## Updating CI

The image tag needs updated in `.gitlab/os-linux.yml`.
