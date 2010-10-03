
%.pdf: %.ps
	ps2pdf $*.ps $*.pdf
	evince $*.pdf &

%.ps: %.dvi
	dvips -o $*.ps $*.dvi

%.dvi: %.tex  
	latex $*.tex
	latex $*.tex

%.eps: %.gp
	gnuplot $<

all: navguide-intro.pdf

clean:
	rm -f *.pdf *.ps *.dvi *.log *.aux *.bbl *.blg *~ *.brf





