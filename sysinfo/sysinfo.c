#include <err.h>
#include <stdbool.h>
#include <unistd.h>

#include <libnvpair.h>

/* included at compile time */
extern void sysinfo_bootparams(nvlist_t *);
extern void sysinfo_uname(nvlist_t *);
extern void sysinfo_smartdc(nvlist_t *);
extern void sysinfo_smbios(nvlist_t *);
extern void sysinfo_uptime(nvlist_t *);
extern void sysinfo_sysconf(nvlist_t *);
extern void sysinfo_zfs(nvlist_t *);
extern void sysinfo_disks(nvlist_t *);
extern void sysinfo_kstat(nvlist_t *);
extern void sysinfo_network(nvlist_t *);

static struct {
	boolean_t opt_f; /* -f, force cache update */
	boolean_t opt_u; /* -u, update cache silently */
} opts;

void usage(FILE *f) {
	fprintf(f, "Usage: sysinfo [-fhu]\n");
	fprintf(f, "\n");
	fprintf(f, "Options\n");
	fprintf(f, "  -f        force a cache update and output data\n");
	fprintf(f, "  -h        print this message and exit\n");
	fprintf(f, "  -u        force a cache update and output nothing\n");
}

int main(int argc, char **argv) {
	nvlist_t *nvl;
	int opt;

	opts.opt_f = B_FALSE;
	opts.opt_u = B_FALSE;
	while ((opt = getopt(argc, argv, "fhu")) != -1) {
		switch (opt) {
		case 'f':
			opts.opt_f = B_TRUE;
			break;
		case 'h':
			usage(stdout);
			return (0);
		case 'u':
			opts.opt_u = B_TRUE;
			break;
		default:
			usage(stderr);
			return (1);
		}
	}

	if (nvlist_alloc(&nvl, NV_UNIQUE_NAME, 0) != 0) {
		warn("nvlist_alloc");
		return 1;
	}

	// XXX this is hardcoded to be true for some reason
	fnvlist_add_boolean(nvl, "VM Capable");

	sysinfo_bootparams(nvl);
	sysinfo_uname(nvl);
	sysinfo_smartdc(nvl);
	sysinfo_smbios(nvl);
	sysinfo_uptime(nvl);
	sysinfo_sysconf(nvl);
	sysinfo_zfs(nvl);
	sysinfo_disks(nvl);
	sysinfo_kstat(nvl);
	sysinfo_network(nvl);

	nvlist_print_json(stdout, nvl);
	printf("\n");
	//nvlist_print(stdout, nvl);

	nvlist_free(nvl);
	return 0;
}
