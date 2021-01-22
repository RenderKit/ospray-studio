PANDOCFLAGS = --highlight-style pygments -f gfm -t gfm
HEAD = components/title.markdown components/toc.markdown

.PHONY: all
all: website readme

.PHONY: website
website: index.md quickstart.md api.md gallery.md

%.md:
	pandoc $(PANDOCFLAGS) -o $@ $^

index.md: $(HEAD)
index.md: components/overview.markdown

quickstart.md: $(HEAD)
quickstart.md: components/download.markdown
quickstart.md: components/build.markdown
quickstart.md: components/quickstart.markdown

api.md: $(HEAD)
api.md: components/developer.markdown
api.md: components/extension.markdown
api.md: components/scenegraph.markdown

gallery.md: $(HEAD)
gallery.md: components/gallery.markdown

.PHONY: readme
readme: components/title.markdown components/release.markdown components/overview.markdown components/build.markdown
	pandoc $(PANDOCFLAGS) -o ../README.md $^

.PHONY: clean
clean:
	-@rm -f *.md
	-@rm -f ../README.md
