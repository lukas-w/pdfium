# PDFium Infrastructure

[TOC]

This document describes PDFium's continuous integration (CI), Commit Queue (CQ),
and build configuration infrastructure.

## Waterfall

The current health of the source tree can be found
[here](https://ci.chromium.org/p/pdfium/g/main/console).

## `infra/config` Branch

LUCI project configurations for PDFium's CI and CQ builders live on the
[`infra/config` branch](https://pdfium.googlesource.com/pdfium/+/refs/heads/infra/config),
which is separate from the `main` branch. Code reviews for this branch are on
Gerrit under
[`project:pdfium branch:infra/config`](https://pdfium-review.googlesource.com/q/project:pdfium+branch:infra/config).

To check out the `infra/config` branch locally in an existing PDFium repository:

```shell
git fetch origin refs/heads/infra/config
git checkout -b my_branch origin/infra/config
```

Note that the `infra/config` branch does not have a `DEPS` file, so there is no
need to run `gclient sync`.

When uploading CLs from the `infra/config` branch, be sure to run
`git branch --set-upstream-to origin/infra/config`.

### Builder Configurations

`main.star` defines the builders, dimensions, properties, console views, and CQ
verifiers using Starlark. Running `main.star` regenerates the LUCI configuration
files under `generated/`:

```shell
./main.star
```

Do not edit the files in `generated/` directly. Always modify `main.star` and
regenerate the configuration files.

## Build Recipes

The build recipes that control how bots check out, configure, compile, and test
PDFium live in the
[`chromium/tools/build`](https://chromium.googlesource.com/chromium/tools/build)
repository.

The PDFium recipe is located at
[`recipes/recipes/pdfium.py`](https://chromium.googlesource.com/chromium/tools/build/+/refs/heads/main/recipes/recipes/pdfium.py).
After modifying the recipe, run `recipes/recipes.py test train` to run the
tests and update the expectation files under `recipes/recipes/pdfium.expected/`:

```shell
./recipes/recipes.py test train
```

### Order of changes

When a task involves updates to recipes and builder configurations, usually one
should update the recipes first with a default that is effectively a no-op, then
update the builder config to active the non-default behavior. e.g.
[recipe change](https://crrev.com/c/7588614) and
[builder change](https://pdfium-review.googlesource.com/c/149210).
