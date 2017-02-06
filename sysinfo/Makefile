NICTAG_PATH = ../common/nictag

LIBS = -lnvpair \
       -ldevinfo \
       -lsmbios \
       -lzfs \
       -lkstat \
       -ldladm \
       -lsocket \
       -lnsl \
       -lscf \
       -lnictag

CFLAGS = -Wall

DEPS_OBJ = sysinfo_bootparams.o \
	   sysinfo_uname.o \
	   sysinfo_smartdc.o \
	   sysinfo_smbios.o \
	   sysinfo_uptime.o \
	   sysinfo_sysconf.o \
	   sysinfo_zfs.o \
	   sysinfo_disks.o \
	   sysinfo_network.o \
	   sysinfo_kstat.o

#sysinfo: sysinfo.c $(DEPS_OBJ)
#	$(CC) -o $@ $^ $(LIBS) $(CFLAGS)

sysinfo: CFLAGS += -L$(NICTAG_PATH)
sysinfo: CFLAGS += -I$(NICTAG_PATH)
sysinfo: CFLAGS += -Wl,-rpath=$(NICTAG_PATH)
sysinfo: sysinfo.c $(DEPS_OBJ)
	(cd $(NICTAG_PATH) && make)
	$(CC) -o $@ $^ $(LIBS) $(CFLAGS)

%.o: %.c
	$(CC) -c -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
	rm -f sysinfo sysinfo *.o
