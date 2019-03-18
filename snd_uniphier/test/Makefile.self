
CC      ?= gcc
CFLAGS  ?= \
	-O2 -g -Wall
LDFLAGS ?= \
	-lpthread \
	-lasound

INSTALL ?= install
MKDIR   ?= mkdir
RM      ?= rm

TARGETS = alsa_beep alsa_play alsa_seek alsa_capture \
	alsa_vol alsa_hwdep hw_params

all: $(TARGETS)

include Makefile.common

install: all
	$(MKDIR) -p /usr/local/bin/
	$(INSTALL) $(TARGETS) /usr/local/bin/

clean:
	$(RM) -f $(TARGETS)
	$(RM) -f $(ALSA_BEEP_OBJS)
	$(RM) -f $(ALSA_PLAY_OBJS)
	$(RM) -f $(ALSA_SEEK_OBJS)
	$(RM) -f $(ALSA_CAPTURE_OBJS)
	$(RM) -f $(ALSA_VOL_OBJS)
	$(RM) -f $(ALSA_HWDEP_OBJS)
	$(RM) -f $(HW_PARAMS_OBJS)
	$(RM) -f $(APCMOUT_OBJS)

distclean: clean

.PHONY: all install clean distclean
