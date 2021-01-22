# OSPRay Studio documentation

This directory contains the individual component files for `README.md` and for
the project website.

## Layout

The readme and website are comprised of various component Markdown files. The
`Makefile` will always have the latest layout for each page on the website.

For example, at time of writing, the API documentation page is built by
combining the following components in this order:

* `title.markdown` - Page title
* `toc.markdown` - Table of contents
* `developer.markdown` - Section title
* `extension.markdown` - Extending Studio with plugins and widgets
* `scenegraph.markdown` - Scenegraph API

## Updating documentation

On release, make sure to update `components/release.markdown`.

If you are just adding to existing content, such as new API features:

1. Edit the appropriate component file in the `components/` directory
   (e.g. `scenegraph.markdown` for scenegraph API changes)
2. Run `make` inside this directory to build the new page(s)
3. Commit the updated files, including the generated files

If you are adding a new component file or a new generated file:

1. Create the component files in `components/` with a `.markdown` extension
   (this differentiates generated files from hand-written files!)
  * If you are creating a new page, add it to the table of contents in
    `toc.markdown`
2. Edit the `Makefile` as needed
  * Add the name of the generated file to the `website` target if you are
    creating a new page
  * Edit the appropriate target or create a new target to include your
    component file(s)
3. Run `make` inside this directory to build the new page(s)
4. Commit the updated files, including the generated files

If you are adding images to the gallery, add them directly to OSPRay's GitHub
Pages repo.
