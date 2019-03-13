# This Makefile is not needed to build the package, it just provides
# convenient way for developer to make ps/pdf/pcl from manpages  --kir.

MANS = vzctl.8 vz.5 ve.conf.5 networks_classes.5 \
 vz-start.5 vz-stop.5 \
 vz-net_add.5 vz-net_del.5  \
 vz-create_prvt.5 vz-postinst.5 \
 vzpurge.8 vzgetpa.8  ubclogd.8 vznetstat.8 vzsplit.8 vzcpucheck.8 \
 vzpid.8 vzcfgscale.8 vzcfgvalidate.8 \
 vzauth.8 vztactl.8 vzlist.8 \
 vzdiskcheck.8 vzeventd.8

PSS = $(patsubst %, %.ps, $(MANS))
PDFS = $(patsubst %, %.pdf, $(MANS))
PCLS = $(patsubst %, %.pcl, $(MANS))
TXTS = $(patsubst %, %.txt, $(MANS))
GROFF = groff
#GROFF_OPTIONS = -Tps -t -msafer -man
# below is for A4 page size, requires a4.tmac
GROFF_OPTIONS = -Tps -t -ma4 -msafer -man -rC1 -rD1 -rS11
GROFF_TEXT_OPTIONS = -Tlatin1 -t -ma4 -msafer -man -rC1 -rD1 -rS11
GROFF_PCL_OPTIONS = -Tlj4 -t -ma4 -msafer -man -rC1 -rD1 -rS11 -P-pa4 -P-d1
PS2PDF = ps2pdf

all:

ps: $(PSS)

all.ps: $(MANS)
	$(GROFF) $(GROFF_OPTIONS) $(MANS) > all.ps

all.pcl: $(MANS)
	$(GROFF) $(GROFF_PCL_OPTIONS) $(MANS) > all.pcl

pdf: $(PDFS)

txt: $(TXTS)

print: manual.pcl
	lpr manual.pcl

clean: clean-ps clean-pdf clean-pcl clean-txt
	rm -f toc.man toc.ps all.ps manual.ps manual.pdf manual.pcl

clean-pdf:
	rm -f $(PDFS)

clean-ps:
	rm -f $(PSS)

clean-pcl:
	rm -f $(PCLS)

clean-txt:
	rm -f $(TXTS)


%.5.ps: %.5
	$(GROFF) $(GROFF_OPTIONS) $< > $@

%.8.ps: %.8
	$(GROFF) $(GROFF_OPTIONS) $< > $@

%.pdf: %.ps
	$(PS2PDF) $<

# pcl - for HP LaserJet and compatible printers
%.5.pcl: %.5
	$(GROFF) $(GROFF_PCL_OPTIONS) $< > $@

%.8.pcl: %.8
	$(GROFF) $(GROFF_PCL_OPTIONS) $< > $@

# text with overstriking
%.5.txt.os: %.5
	$(GROFF) $(GROFF_TEXT_OPTIONS) $< > $@
%.8.txt.os: %.8
	$(GROFF) $(GROFF_TEXT_OPTIONS) $< > $@

# to remove overstrike
%.txt: %.txt.os
	sed 's/.//g' < $< > $@ && rm -f $<

toc.man: toc.man.in all.ps Makefile
	sed -ne '1,/@TOC@/p' < toc.man.in | sed '/@TOC@/d' > $@
	./toc.sh all.ps >> $@
	sed -ne '/@TOC@/,$$p' < toc.man.in | sed '/@TOC@/d' >> $@

toc.ps: toc.man
	$(GROFF) -Tps -t -ma4 -msafer $< > $@

manual.pcl: toc.man $(MANS)
	$(GROFF) $(GROFF_PCL_OPTIONS) $^ > $@

manual.ps: toc.man $(MANS)
	$(GROFF) $(GROFF_OPTIONS) $^ > $@

.PHONY: clean clean-ps clean-pdf
