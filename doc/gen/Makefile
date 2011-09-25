SOURCES=ad-note.tex echo.tex distort.tex dynamic.tex reverb.tex chorus.tex
OBJECTS=$(SOURCES:.tex=.png)

all:$(OBJECTS)

%.png: %.tex
	pslatex $<
	convert $(<:.tex=.dvi) -resize 200% -trim $@
	rm $(<:.tex=.dvi) $(<:.tex=.log) $(<:.tex=.aux)

.PHONY: clean
clean:
	rm -f *.png *~

