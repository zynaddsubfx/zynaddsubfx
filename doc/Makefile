
.PHONY: subdirs gen

all: gen-files man xhtml chunked pdf

gen-files:
	cd gen; make


xhtml: zynaddsubfx.txt envelope.txt intro.txt lfo.txt
	asciidoc -a numbered -a toc -b html5 \
		-a latexmath \
		zynaddsubfx.txt

chunked:
	asciidoc --doctype=article   -b docbook \
	          zynaddsubfx.txt
	rm -rf ./zynaddsubfx.chunked
	mkdir ./zynaddsubfx.chunked
	rm -f "./zynaddsubfx.chunked/*.html"
	ln -s ../images/ zynaddsubfx.chunked/images
	xsltproc --stringparam html.stylesheet "./docbook-xsl.css" \
	         --stringparam callout.graphics 0 \
		 --stringparam navig.graphics 0 \
		 --stringparam admon.textlabel 1 \
		 --stringparam admon.graphics 0 \
		 --nonet \
	         --stringparam base.dir "zynaddsubfx.chunked/" \
	         "/etc/asciidoc/docbook-xsl/chunked.xsl" \
		 "./zynaddsubfx.xml" \

pdf:
	a2x -f pdf zynaddsubfx.txt

man: zynaddsubfx.1.txt
	a2x -f manpage zynaddsubfx.1.txt

clean:
	rm -f *~ *html *pdf *xml *tex *log zynaddsubfx.chunked/images
	rm -r -f zynaddsubfx.chunked/

