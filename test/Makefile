include ../config.mk

CFLAGS += -I.. -Dstatic= -ffunction-sections
LDFLAGS += -Wl,-z,muldefs

GS = gs
GSFLAGS = -sDEVICE=nullpage -dBATCH -dQUIET -I../res

CTESTS = ct_strecat ct_deutf ct_takecsi ct_u2opts ct_fonts1 ct_fonts2 \
	ct_font_name_match
PTESTS = pt_deutf pt_findglyph pt_rangew pt_substcode pt_uniname pt_ushow \
	pt_term_u pt_term_x pt_printf

all: ctests
run: crun prun

ptests:
ctests: $(CTESTS)
prun: ptests $(patsubst %,%.out,$(PTESTS))
crun: ctests $(patsubst %,%.out,$(CTESTS))

clean:
	rm -f *.o *.out
	rm -f $(CTESTS)

pt_%.out: pt_%.ps
	$(GS) $(GSFLAGS) $< > $@
	@grep -q FAIL $@ && (cat $@; rm $@) || true

ct_%.out: ct_%
	./$< > $@
	@grep -q FAIL $@ && (cat $@; rm $@) || true

ct_%.o: ct_%.c
	$(CC) $(CFLAGS) -o $@ -c $<

u2ps_%.o: ../u2ps_%.c
	$(CC) $(CFLAGS) -o $@ -c $<

psfrem_%.o: ../psfrem_%.c
	$(CC) $(CFLAGS) -o $@ -c $<

psfrem: ../psfrem
	cp $< $@

../psfrem:
	$(MAKE) -C .. $(notdir $@)

warn.o: ../warn.c
	$(CC) $(CFLAGS) -o $@ -c $<

ct_%: ct_%.o
	$(CC) -o $@ $(filter %.o,$^) $(LDFLAGS)

ct_deutf: ct_deutf.o u2ps_unicode.o

ct_takecsi: ct_takecsi.o globals.o u2ps_term.o u2ps_page.o u2ps_termcsi.o u2ps_pswr.o \
	u2ps_unicode.o u2ps_data.o warn.o

ct_strecat: ct_strecat.o psfrem_util.o

ct_u2opts: ct_u2opts.o u2ps_opts.o u2ps_data.o warn.o

ct_fonts1: ct_fonts1.o u2ps_opts.o u2ps_data.o globals.o warn.o
ct_fonts2: ct_fonts2.o u2ps_opts.o u2ps_data.o globals.o warn.o

ct_font_name_match: ct_font_name_match.o u2ps_opts.o u2ps_data.o globals.o warn.o

pt_fstat.out: GSFLAGS += -dWRITESYSTEMDICT
