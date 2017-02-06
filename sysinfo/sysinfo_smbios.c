#include <err.h>
#include <smbios.h>
#include <string.h>

#include <libnvpair.h>

static void do_system_common(smbios_info_t *info, nvlist_t *nvl) {
	if (info->smbi_manufacturer[0] != '\0')
		fnvlist_add_string(nvl, "Manufacturer", info->smbi_manufacturer);
	if (info->smbi_product[0] != '\0')
		fnvlist_add_string(nvl, "Product", info->smbi_product);
	if (info->smbi_version[0] != '\0')
		fnvlist_add_string(nvl, "HW Version", info->smbi_version);
	if (info->smbi_serial[0] != '\0')
		fnvlist_add_string(nvl, "Serial Number", info->smbi_serial);
	if (info->smbi_asset[0] != '\0')
		fnvlist_add_string(nvl, "Asset Tag", info->smbi_asset);
	if (info->smbi_location[0] != '\0')
		fnvlist_add_string(nvl, "Location Tag", info->smbi_location);
	if (info->smbi_part[0] != '\0')
		fnvlist_add_string(nvl, "Part Number", info->smbi_part);
}

static void do_system(smbios_system_t *s, nvlist_t *nvl) {
	int i;
	char uuid[256];
	char buf[3];

	uuid[0] = '\0';
	for (i = 0; i < s->smbs_uuidlen; i++) {
		snprintf(buf, sizeof (buf), "%02x", s->smbs_uuid[i]);
		strncat(uuid, buf, sizeof (buf));
		if (i == 3 || i == 5 || i == 7 || i == 9)
			strcat(uuid, "-");
	}

	fnvlist_add_string(nvl, "UUID", uuid);
	fnvlist_add_string(nvl, "SKU Number", s->smbs_sku);
	fnvlist_add_string(nvl, "HW Family", s->smbs_family);
}

static void do_processor_common(smbios_info_t *info, nvlist_t *nvl) {
	if (info->smbi_version[0] != '\0') {
		// CPU type has trailing spaces - trim it
		char type[256];
		int i;
		strncpy(type, info->smbi_version, sizeof (type));
		i = sizeof (type) - 1;
		type[i] = '\0';
		for ( ; (type[i] == '\0' || type[i] == ' ') && i > 0; i--)
			type[i] = '\0';

		fnvlist_add_string(nvl, "CPU Type", type);
	}
}

static void do_processor(smbios_processor_t *p, nvlist_t *nvl) {
	if (p->smbp_corecount != 0) {
		if (p->smbp_corecount != 0xff || p->smbp_corecount2 == 0)
			fnvlist_add_int32(nvl, "CPU Total Cores", p->smbp_corecount);
		else
			fnvlist_add_int32(nvl, "CPU Total Cores", p->smbp_corecount2);
	} else {
		// TODO warning
	}
}

static int do_struct(smbios_hdl_t *shp, const smbios_struct_t *sp, void *arg) {
	nvlist_t *nvl = (nvlist_t *)arg;
	smbios_info_t info;
	smbios_system_t s;
	smbios_processor_t p;

	switch (sp->smbstr_type) {
	case SMB_TYPE_SYSTEM:
		if (smbios_info_common(shp, sp->smbstr_id, &info) == 0)
			do_system_common(&info, nvl);
		smbios_info_system(shp, &s);
		do_system(&s, nvl);
		break;
	case SMB_TYPE_PROCESSOR:
		if (smbios_info_common(shp, sp->smbstr_id, &info) == 0)
			do_processor_common(&info, nvl);
		smbios_info_processor(shp, sp->smbstr_id, &p);
		do_processor(&p, nvl);
		break;
	}

	return 0;
}

void sysinfo_smbios(nvlist_t *root_nvl) {
	smbios_hdl_t *shp;
	int err;

	if ((shp = smbios_open(NULL, SMB_VERSION, NULL, &err)) == NULL) {
		warn("smbios_open");
		return;
	}

	smbios_iter(shp, do_struct, root_nvl);
	smbios_close(shp);
}
