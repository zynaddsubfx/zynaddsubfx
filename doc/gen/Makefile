SOURCES=velf.tex ad-note.tex echo.tex distort.tex dynamic.tex reverb.tex chorus.tex
OBJECTS=$(SOURCES:.tex=.png)

all:$(OBJECTS)

%.png: %.tex
	pdflatex -shell-escape $<
	convert -trim -density 300 $(<:.tex=.pdf) $@
	rm -f $(<:.tex=.pdf) $(<:.tex=.log) $(<:.tex=.aux) $(<:.tex=-pic*) $(<:.tex=.pgf-plot*)

.PHONY: clean
clean:
	rm -f *.png *~ *.aux *.auxlock *.table *.log *.gnuplot *.dpth *.dep

