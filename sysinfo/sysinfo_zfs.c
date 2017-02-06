#include <err.h>
#include <sys/utsname.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <libscf.h>
#include <libscf_priv.h>

#include <libnvpair.h>
#include <libzfs.h>

static char *smartdc_fmri = "system/smartdc/init";

static void
get_zpool_name(char *buf, size_t len)
{
	scf_handle_t *h = NULL;
	scf_propertygroup_t	*pg = NULL;
	scf_property_t	*prop = NULL;
	scf_value_t	*value = NULL;
	scf_scope_t	*sc = NULL;
	scf_service_t	*svc = NULL;


	/* figure out the zpool name */
	h = scf_handle_create(SCF_VERSION);
	if (h == NULL)
		goto done;

	sc = scf_scope_create(h);
	svc = scf_service_create(h);
	pg = scf_pg_create(h);
	prop = scf_property_create(h);
	value = scf_value_create(h);

	/* bind scf handle to the running svc.configd daemon */
	if (scf_handle_bind(h) == -1)
		goto done;

	/* get the scope of the localhost in the current repository */
	if (scf_handle_get_scope(h, SCF_SCOPE_LOCAL, sc) == -1)
		goto done;

	/* get the service "network/isns_server" within the scope */
	if (scf_scope_get_service(sc, smartdc_fmri, svc) == -1)
		goto done;

	/* get the property group "config" within the given service */
	if (scf_service_get_pg(svc, "config", pg) == -1)
		goto done;

	/* Now get the properties. */
	if (scf_pg_get_property(pg, "zpool", prop) == -1)
		goto done;

	if (scf_property_get_value(prop, value) == -1)
		goto done;

	buf[0] = '\0';
	if (scf_value_get_astring(value, buf, len) == -1)
		goto done;

done:
	scf_scope_destroy(sc);
	scf_service_destroy(svc);
	scf_pg_destroy(pg);
	scf_property_destroy(prop);
	scf_value_destroy(value);
	scf_handle_destroy(h);
}

static void
recurse(nvlist_t *nvl, zpool_handle_t *zp, libzfs_handle_t *zh, char *alldevices)
{
	nvlist_t **child;
	uint_t c, children;
	if (nvlist_lookup_nvlist_array(nvl, ZPOOL_CONFIG_CHILDREN, &child, &children) != 0)
		children = 0;

	for (c = 0; c < children; c++) {
		uint64_t islog = B_FALSE, ishole = B_FALSE;
		char *vname;

		/* Don't print logs or holes here */
		(void) nvlist_lookup_uint64(child[c], ZPOOL_CONFIG_IS_LOG, &islog);
		(void) nvlist_lookup_uint64(child[c], ZPOOL_CONFIG_IS_HOLE, &ishole);
		if (islog || ishole)
			continue;

		vname = zpool_vdev_name(zh, zp, child[c], B_TRUE);
		if (alldevices[0] == '\0')
			snprintf(alldevices, 2048, "%s", vname);
		else
			snprintf(alldevices, 2048, ",%s", vname);
		recurse(child[c], zp, zh, alldevices);
		free(vname);
	}
}

static char *
get_zpool_profile(nvlist_t *nvl)
{
	nvlist_t **child;
	uint_t children;
	char *profile;

	if (nvlist_lookup_nvlist_array(nvl, ZPOOL_CONFIG_CHILDREN, &child, &children) != 0)
		children = 0;
	if (children < 1)
		return NULL;

	nvlist_lookup_string(child[0], ZPOOL_CONFIG_TYPE, &profile);
	if (strcmp(profile, "disk") == 0)
		profile = "striped";
	return profile;
}

void
sysinfo_zfs(nvlist_t *root_nvl)
{
	char zpool[2048];
	libzfs_handle_t *zh;
	zpool_handle_t *zp;
	zfs_handle_t *zds;
	nvlist_t *config;
	nvlist_t *nvroot;
	int size = 0;
	char alldevices[2048];

	get_zpool_name(zpool, sizeof (zpool));

	fnvlist_add_string(root_nvl, "Zpool", zpool);

	//XX

	/* get ZFS and zpool information */
	if ((zh = libzfs_init()) == NULL) {
		warn("libzfs_init");
		return;
	}

	if ((zp = zpool_open(zh, zpool)) == NULL) {
		warn("zpool_open");
		return;
	}

	if ((zds = zfs_open(zh, zpool, ZFS_TYPE_FILESYSTEM)) == NULL) {
		warn("zfs_open");
		return;
	}

	/*
	 * this is called "Zpool Creation", but really this is the creation
	 * time of the root dataset- A carry-over from the bash sysinfo script
	 */
	fnvlist_add_int32(root_nvl, "Zpool Creation",
	    zfs_prop_get_int(zds, ZFS_PROP_CREATION));

	/*
	 * this is called "Zpool Size in Gib", but really this is the used +
	 * available space of the root dataset- A carry-over from the bash
	 * sysinfo script as well.  Note that the "Size" property of the zpool
	 * itself will yield a different value than this will generate.
	 */
	size += zfs_prop_get_int(zds, ZFS_PROP_USED) / 1024 / 1024 / 1024;
	size += zfs_prop_get_int(zds, ZFS_PROP_AVAILABLE) / 1024 / 1024 / 1024;
	fnvlist_add_int32(root_nvl, "Zpool Size in GiB", size);

	/*
	 * find disks in the zpool
	 */
	alldevices[0] = '\0';
	config = zpool_get_config(zp, NULL);
	nvlist_lookup_nvlist(config, ZPOOL_CONFIG_VDEV_TREE, &nvroot);
	recurse(nvroot, zp, zh, alldevices);
	fnvlist_add_string(root_nvl, "Zpool Disks", alldevices);

	/*
	 * get zpool profile
	 */
	fnvlist_add_string(root_nvl, "Zpool Profile", get_zpool_profile(nvroot));

done:
	zpool_close(zp);
	zfs_close(zds);
	libzfs_fini(zh);
}
