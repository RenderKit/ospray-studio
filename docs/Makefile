PANDOCFLAGS = --highlight-style pygments -f gfm -t gfm
HEAD = components/title.markdown components/toc.markdown

# by default, build both the website and the readme
.PHONY: all
all: website readme

# the website is comprised of these markdown files, which are generated
# from component files
.PHONY: website
website: index.md quickstart.md api.md gallery.md

# this is the common build command to combine component .markdown files
# into generated .md files. Please do not name component files with the
# .md extension!
%.md:
	pandoc $(PANDOCFLAGS) -o $@ $^

# homepage contains title, table of contents, and overview
index.md: $(HEAD)
index.md: components/overview.markdown

# download, build, and basic running instructions
quickstart.md: $(HEAD)
quickstart.md: components/download.markdown
quickstart.md: components/build.markdown
quickstart.md: components/quickstart.markdown

# how to extend Studio and API docs
api.md: $(HEAD)
api.md: components/developer.markdown
api.md: components/extension.markdown
api.md: components/scenegraph.markdown

# shiny
gallery.md: $(HEAD)
gallery.md: components/gallery.markdown

# basic readme with release version and build instructions
.PHONY: readme
readme: components/title.markdown components/release.markdown components/overview.markdown components/build.markdown
	pandoc $(PANDOCFLAGS) -o ../README.md $^

.PHONY: clean
clean:
	-@rm -f *.md
	-@rm -f ../README.md
