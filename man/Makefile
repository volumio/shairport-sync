all: shairport-sync.1 shairport-sync.html

%.1: %.1.xml
	xmltoman $*.1.xml > $*.tmp && mv $*.tmp $*.1

%.html: %.1.xml
	xsltproc xmltoman.xsl $*.1.xml > $*.tmp && mv $*.tmp $*.html

clean:
	rm shairport-sync.1
	rm shairport-sync.html
