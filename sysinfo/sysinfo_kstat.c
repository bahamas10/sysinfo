#include <err.h>
#include <sys/utsname.h>
#include <string.h>
#include <unistd.h>
#include <kstat.h>

#include <libnvpair.h>

void sysinfo_kstat(nvlist_t *root_nvl) {
	int max_chip_id;
	int *chips = NULL;
	kstat_ctl_t	*kc = NULL;
	kstat_t		*ksp;
	kstat_named_t	*knp;
	int cpus = 0;
	int i;

	if ((kc = kstat_open()) == NULL)
		goto done;

	/* get physical core count */
	max_chip_id = sysconf(_SC_CPUID_MAX);
	if ((chips = calloc(max_chip_id, sizeof (int))) == NULL)
		goto done;

	if ((ksp = kstat_lookup(kc, "cpu_info", -1, NULL)) == NULL)
		goto done;

	for (ksp = kc->kc_chain; ksp; ksp = ksp->ks_next) {
		if (strcmp(ksp->ks_module, "cpu_info") != 0)
			continue;
		if (kstat_read(kc, ksp, NULL) == NULL)
			goto done;

		if ((knp = kstat_data_lookup(ksp, "chip_id")) != NULL)
			chips[knp->value.l]++;
	}

	for (i = 0; i < max_chip_id; i++)
		if (chips[i] > 0)
			cpus++;

	fnvlist_add_int32(root_nvl, "CPU Physical Cores", cpus);
done:
	if (kc != NULL)
		(void) kstat_close(kc);
	free(chips);
}
