NICTAG_PATH = ../common/nictag

CFLAGS = -Wall
LIBS = -lnvpair -lnictag -ldladm

#nictagadm: nictagadm.c
#	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

nictagadm: CFLAGS += -L$(NICTAG_PATH)
nictagadm: CFLAGS += -I$(NICTAG_PATH)
nictagadm: CFLAGS += -Wl,-rpath=$(NICTAG_PATH)
nictagadm: nictagadm.c
	(cd $(NICTAG_PATH) && make)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

.PHONY: clean
clean:
	rm -f nictagadm
