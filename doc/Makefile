
.PHONY: subdirs gen

all: xhtml

xhtml: zynaddsubfx.txt envelope.txt intro.txt lfo.txt
	asciidoc -a numbered -a toc -b xhtml11 \
	    --conf-file=myconf.conf zynaddsubfx.txt

chunked:
	asciidoc --doctype=article   -b docbook \
	         --conf-file=myconf.conf zynaddsubfx.txt
	mkdir ./zynaddsubfx.chunked
	rm -f "./zynaddsubfx.chunked/*.html"
	ln -s ../images/ zynaddsubfx.chunked/images
	cd "."
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
	asciidoc -b latex --conf-file=myconf.conf zynaddsubfx.txt
	grep -v figure zynaddsubfx.tex | \
	sed -e 's~^\\includegraphics~\\par\\noindent\{\} \\includegraphics~' | \
	sed -e 's~includegraphics\[\]~includegraphics[scale=0.5]~' > tmp.tex
	mv tmp.tex zynaddsubfx.tex
	latex zynaddsubfx.tex

clean:
	rm -f *~ *html *pdf *xml *tex *log zynaddsubfx.chunked/images
	rm -r -f zynaddsubfx.chunked/

