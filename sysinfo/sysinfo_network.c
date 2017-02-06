#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <kstat.h>

#include <libdladm.h>
#include <libdllink.h>
#include <libdlaggr.h>
#include <libnvpair.h>

#include <libnictag.h>

static struct sysinfonet {
	nvlist_t *nictags;
	nvlist_t *ret;
};

static void
mac2str(const unsigned char *macaddr, char *buf)
{
	int i, n;
	n = sprintf(buf, "%02x", *macaddr++);
	for (i = 0; i < (ETHERADDRL - 1); i++)
		n += sprintf(buf+n, ":%02x", *macaddr++);
}

static boolean_t
str2mac(const char *buf, unsigned int **macaddr)
{
	return sscanf(buf, "%x:%x:%x:%x:%x:%x%c",
		macaddr[0],
		macaddr[1],
		macaddr[2],
		macaddr[3],
		macaddr[4],
		macaddr[5]) == 6 ? B_TRUE : B_FALSE;
}

static boolean_t
phys_macaddr(void *arg, dladm_macaddr_attr_t *attr)
{
	struct sysinfonet *oo = (struct sysinfonet *)arg;
	nvlist_t *int_nvl = oo->ret;
	nvlist_t *nictags = oo->nictags;
	nvpair_t *curr;
	char macstr[ETHERADDRL * 3];
	char **tags;
	int i = 0;

	tags = malloc(sizeof (char *) * 1024);
	if (tags == NULL)
		return B_TRUE; // TODO better error handling

	mac2str(attr->ma_addr, macstr);
	fnvlist_add_string(int_nvl, "MAC Address", macstr);

	// loop each nic tag and find all the relevant ones
	// TODO normalize macs in libnictag
	for (curr = nvlist_next_nvpair(nictags, NULL); curr;
	    curr = nvlist_next_nvpair(nictags, curr)) {
		char *name = nvpair_name(curr);
		char *mac;
		nvpair_value_string(curr, &mac);

		if (strcmp(mac, macstr) == 0) {
			// match
			tags[i] = name;
			if (++i == 1024)
				return B_TRUE; // TODO better error handling
		}
	}
	fnvlist_add_string_array(int_nvl, "NIC Names", tags, i);

	return B_TRUE;
}

static void
get_link_state(char *link, nvlist_t *nvl)
{
	kstat_ctl_t	*kcp;
	kstat_t		*ksp;
	kstat_named_t		*knp;
	uint32_t linkstate;
	char linkstate_str[256];

	if ((kcp = kstat_open()) == NULL)
		goto done;

	if ((ksp = kstat_lookup(kcp, "link", 0, link)) == NULL)
		goto done;

	if (kstat_read(kcp, ksp, NULL) == -1)
		goto done;

	if ((knp = kstat_data_lookup(ksp, "link_state")) == NULL)
		goto done;

	linkstate = knp->value.ui32;
	dladm_linkstate2str(linkstate, linkstate_str);

	fnvlist_add_string(nvl, "Link Status", linkstate_str);
done:
	kstat_close(kcp);
}

static int
show_phys(dladm_handle_t handle, datalink_id_t linkid, void *arg)
{
	struct sysinfonet *o = (struct sysinfonet *)arg;
	struct sysinfonet oo;
	nvlist_t *net_nvl = o->ret;
	nvlist_t *int_nvl = NULL;
	char link[MAXLINKNAMELEN];
	uint32_t		flags;
	datalink_class_t	class;
	uint32_t		media;

	if (nvlist_alloc(&int_nvl, NV_UNIQUE_NAME, 0) != 0) {
		warn("nvlist_alloc");
		goto done;
	}

	/* get mac address */
	if (dladm_datalink_id2info(handle, linkid, &flags, &class,
	    &media, link, MAXLINKNAMELEN) != DLADM_STATUS_OK)
		goto done;

	if (class != DATALINK_CLASS_PHYS)
		goto done;

	oo.ret = int_nvl;
	oo.nictags = o->nictags;
	dladm_walk_macaddr(handle, linkid, &oo, phys_macaddr);

	/* get link state */
	get_link_state(link, int_nvl);

done:
	nvlist_add_nvlist(net_nvl, link, int_nvl);
	nvlist_free(int_nvl);
	return (DLADM_WALK_CONTINUE);
}

static nvlist_t *
get_nictags()
{
	char *buf;
	nvlist_t *config;
	nvlist_t *nictags;

	buf = nictag_read_config();
	if (buf == NULL)
		return NULL;

	config = nictag_parse_config(buf);
	if (config == NULL) {
		free(buf);
		return NULL;
	}
	free(buf);

	nictags = nictag_get_tags(config);
	nvlist_free(config);
	return nictags;
}

void
sysinfo_network(nvlist_t *root_nvl)
{
	nvlist_t *net_nvl = NULL;
	nvlist_t *int_nvl = NULL;
	dladm_handle_t handle = NULL;
	uint32_t	flags = DLADM_OPT_ACTIVE;
	struct ifaddrs *ifap, *ifa;
	nvlist_t *nictags = NULL;
	struct sysinfonet o;

	if ((nictags = get_nictags()) == NULL) {
		warn("get_nictags");
		goto done;
	}

	if (dladm_open(&handle) != DLADM_STATUS_OK)
		goto done;

	net_nvl = fnvlist_alloc();

	/*
	 * get interfaces and mac addresses (dladm show-phys -m)
	 * effectively
	 */
	o.nictags = nictags;
	o.ret = net_nvl;
	dladm_walk_datalink_id(show_phys, handle, &o,
	    DATALINK_CLASS_PHYS, DATALINK_ANY_MEDIATYPE, flags);

	/*
	 * get ip addresses by correlating data with getifaddrs
	 */
	getifaddrs(&ifap);
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family == AF_INET) {
			struct sockaddr_in *sa = (struct sockaddr_in *) ifa->ifa_addr;
			char *link = ifa->ifa_name;
			char *addr = inet_ntoa(sa->sin_addr);
			if (nvlist_lookup_nvlist(net_nvl, link, &int_nvl) == 0)
				fnvlist_add_string(int_nvl, "ip4addr", addr);
		}
	}
	freeifaddrs(ifap);

	fnvlist_add_nvlist(root_nvl, "Network Interfaces", net_nvl);

done:
	nvlist_free(net_nvl);
	nvlist_free(nictags);
	if (handle)
		dladm_close(handle);
}
