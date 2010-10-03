SUBDIRS = lcmtypes common model gtk_util image monitor audio jpegcodec libmser libsift libsift2 libfast libsurf features collector grabber xsens guidance viewer dead-reckon benchmark evaluation logger

all: 
	@for f in $(SUBDIRS) ; do $(MAKE) -C $$f all || exit 1 ; done

.PHONY: doc

doc:
	$(MAKE) -C doc doc

clean:
	@for f in $(SUBDIRS) ; do $(MAKE) -C $$f clean ; done

clean-doc:
	$(MAKE) -C doc clean

