TYPE = dir

BASEDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

SUBDIRS := json memcheck

all: $(OUTDIR) $(SUBDIRS)

$(OUTDIR): 
	if [ ! -d ${OUTDIR} ]; then install -d ${OUTDIR}; fi

clean: 
	if [ -d ${OUTDIR}/bin ]; then rmdir $(OUTDIR)/bin; fi
	if [ -d ${OUTDIR}/lib ]; then rmdir $(OUTDIR)/lib; fi
	if [ -d ${OUTDIR} ]; then rmdir $(OUTDIR); fi

.PHONY: all clean 

include $(BASEDIR)/tools/makefiles/$(TYPE).mk
