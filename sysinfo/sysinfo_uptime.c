#include <err.h>
#include <string.h>
#include <unistd.h>
#include <utmpx.h>
#include <sys/stat.h>
#include <time.h>

#include <libnvpair.h>

/*
 * most of this lifted from w.c
 */
void sysinfo_uptime(nvlist_t *root_nvl) {
	struct stat sbuf;
	int entries;
	size_t size;
	struct utmpx *ut, *utmpbegin, *utmpend, *utp;

	/*
	 * read the UTMPX_FILE (contains information about each logged in user)
	 */
	if (stat(UTMPX_FILE, &sbuf) != 0) {
		warn("stat(%s)", UTMPX_FILE);
		return;
	}
	entries = sbuf.st_size / sizeof (struct futmpx);
	size = sizeof (struct utmpx) * entries;
	if ((ut = malloc(size)) == NULL) {
		warn("malloc");
		return;
	}

	(void) utmpxname(UTMPX_FILE);

	utmpbegin = ut;
	utmpend = (struct utmpx *)((char *)utmpbegin + size);

	setutxent();
	while ((ut < utmpend) && ((utp = getutxent()) != NULL))
		(void) memcpy(ut++, utp, sizeof (*ut));
	endutxent();

	for (ut = utmpbegin; ut < utmpend; ut++) {
		if (ut->ut_type != BOOT_TIME)
			continue;
		fnvlist_add_int32(root_nvl, "Boot Time", ut->ut_xtime);
		return;
	}
}
