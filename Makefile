report:
	latexmk -pdf  -shell-escape report.tex 

clean:
	rm -f *~
	rm -rf *.aux
	rm -rf *.bbl
	rm -rf *.blg
	rm -rf *.d
	rm -rf *.fls
	rm -rf *.ilg
	rm -rf *.ind
	rm -rf *.toc*
	rm -rf *.lot*
	rm -rf *.lof*
	rm -rf *.log
	rm -rf *.idx
	rm -rf *.out*
	rm -rf *.nlo
	rm -rf *.nls
	rm -rf *.pdf
	rm -rf *.ps
	rm -rf *.dvi
	rm -rf *.fdb_latexmk
	rm -rf _minted-report
