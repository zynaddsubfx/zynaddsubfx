
.PHONY: subdirs gen

all: gen-files man xhtml chunked pdf

gen-files:
	cd gen; make


xhtml: zynaddsubfx.txt envelope.txt intro.txt lfo.txt
	asciidoctor -a numbered -a toc -b html5 \
		-a latexmath \
		zynaddsubfx.txt

chunked:
	rm -rf ./zynaddsubfx.chunked
	mkdir ./zynaddsubfx.chunked
	rm -f "./zynaddsubfx.chunked/*.html"
	ln -s ../images/ zynaddsubfx.chunked/images
	cp *.txt zynaddsubfx.chunked/
	@curl https://raw.githubusercontent.com/asciidoctor/asciidoctor-extensions-lab/master/lib/multipage-html5-converter.rb -o ./zynaddsubfx.chunked/multipage-html5-converter.rb
	cd ./zynaddsubfx.chunked; asciidoctor -r ./multipage-html5-converter.rb -b multipage_html5 zynaddsubfx.txt
	ln -s zynaddsubfx-chunked.html ./zynaddsubfx.chunked/index.html
	rm ./zynaddsubfx.chunked/multipage-html5-converter.rb ./zynaddsubfx.chunked/*.txt

pdf:
	asciidoctor-pdf zynaddsubfx.txt

man: zynaddsubfx.1.txt
	asciidoctor -b manpage zynaddsubfx.1.txt

clean:
	rm -f *~ *html *pdf *xml *tex *log zynaddsubfx.chunked/images
	rm -r -f zynaddsubfx.chunked/

