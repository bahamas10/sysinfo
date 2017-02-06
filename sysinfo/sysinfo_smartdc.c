#include <err.h>
#include <errno.h>
#include <sys/utsname.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <libnvpair.h>

void sysinfo_smartdc(nvlist_t *root_nvl) {
	FILE *f;
	char version[256];

	/* smartdc version */
	f = fopen("/.smartdc_version", "r");
	if (f != NULL) {
		if (fscanf(f, "%s", version) == 1)
			fnvlist_add_string(root_nvl, "SDC Version", version);
		fclose(f);
	}

	/* sdc setup completion status */
	f = fopen("/var/lib/setup.json", "r");
	if (f == NULL) {
		if (errno == ENOENT)
			fnvlist_add_boolean_value(root_nvl, "Setup", B_FALSE);
	} else {
		// TODO parse JSON and look for the "complete" property
		fclose(f);
	}
}
