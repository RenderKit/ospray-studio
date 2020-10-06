%.html: %.md
	pandoc --highlight-style pygments --standalone -t html --toc -c docs.css --metadata title="OSPRay Studio Scene Graph" -o $@ $<

.PHONY: all html clean

html: scenegraph.html

all: html

clean:
	rm *.html
