#include <err.h>
#include <sys/utsname.h>
#include <string.h>
#include <unistd.h>

#include <libnvpair.h>

void sysinfo_uname(nvlist_t *root_nvl) {
	struct utsname buf;
	char *image;

	if (uname(&buf) == -1) {
		warn("uname(2)");
		return;
	}

	fnvlist_add_string(root_nvl, "System Type", buf.sysname);
	fnvlist_add_string(root_nvl, "Hostname", buf.nodename);

	(void) strtok(buf.version, "_");
	image = strtok(NULL, "_");
	fnvlist_add_string(root_nvl, "Live Image", image);
}
