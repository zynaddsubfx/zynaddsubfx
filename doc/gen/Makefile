all:echo.png ad-note.png

ad-note.png: ad-note.tex
	pslatex ad-note.tex
	convert ad-note.dvi -trim ad-note.png
	rm ad-note.dvi ad-note.log ad-note.aux

echo.png: echo.tex
	pslatex echo.tex
	convert echo.dvi -trim echo.png
	rm echo.dvi echo.log echo.aux
