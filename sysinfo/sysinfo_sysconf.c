#include <err.h>
#include <sys/utsname.h>
#include <string.h>
#include <unistd.h>

#include <libnvpair.h>

void sysinfo_sysconf(nvlist_t *root_nvl) {
	long pagesize = sysconf(_SC_PAGESIZE);
	long npages = sysconf(_SC_PHYS_PAGES);
	int mb = pagesize * npages / 1024 / 1024;
	fnvlist_add_int32(root_nvl, "MiB of Memory", mb);
}
