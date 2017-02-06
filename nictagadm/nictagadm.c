#include <err.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <libdladm.h>
#include <libdllink.h>
#include <libnvpair.h>

#include <libnictag.h>

static struct {
	boolean_t opt_v; /* -v, verbose mode */
} opts;

static void
usage(FILE *f)
{
	fprintf(f, "Usage: nictagadm [opts] <command>\n");
	fprintf(f, "\n");
	fprintf(f, "Manage nictags\n");
	fprintf(f, "\n");
	fprintf(f, "Options\n");
	fprintf(f, "  -h       print this message and exit\n");
	fprintf(f, "  -v       verbose output\n");
	fprintf(f, "\n");
	fprintf(f, "Commands\n");
	fprintf(f, "  add      add a nictag\n");
	fprintf(f, "  delete   delete a nictag\n");
	fprintf(f, "  exists   check if a nictag exists\n");
	fprintf(f, "  list     list nictags\n");
	fprintf(f, "  update   update a nictag\n");
	fprintf(f, "  vms      something\n");
}

static int
walk_etherstub(dladm_handle_t handle, datalink_id_t linkid, void *arg)
{
	char vnic_name[MAXLINKNAMELEN];
	if (dladm_datalink_id2info(handle, linkid, NULL, NULL,
	    NULL, vnic_name, sizeof (vnic_name)) != DLADM_STATUS_OK)
		return (DLADM_STATUS_BADARG);

	printf("etherstub = %s\n", vnic_name);

	return DLADM_WALK_CONTINUE;
}

static int
do_add(int argc, char **argv)
{
	printf("do_add: argc = %d, argv[0] = %s\n", argc, argv[0]);
	return 0;
}

static int
do_delete(int argc, char **argv)
{
	printf("do_delete: argc = %d, argv[0] = %s\n", argc, argv[0]);
	return 0;
}

static int
do_exists(int argc, char **argv)
{
	printf("do_exists: argc = %d, argv[0] = %s\n", argc, argv[0]);
	return 0;
}

static int
do_list(int argc, char **argv)
{
	char *buf;
	dladm_handle_t handle = NULL;
	nvlist_t *config = NULL;
	nvlist_t *nictags = NULL;
	uint32_t flags = DLADM_OPT_ACTIVE;
	char **conf_stubs;

	buf = nictag_read_config();
	if (buf == NULL)
		return 1;

	config = nictag_parse_config(buf);
	if (config == NULL)
		return 1;
	free(buf);

	// get nic tags
	nictags = nictag_get_tags(config);
	if (nictags == NULL)
		return 1;

	nvlist_print_json(stdout, nictags);
	nvlist_free(nictags);
	printf("\n");

	// get etherstubs (in the config)
	conf_stubs = nictag_get_etherstubs(config);
	if (conf_stubs != NULL) {
		char *stub;
		while ((stub = *conf_stubs++) != NULL)
			printf("conf stub = %s\n", stub);
	}
	nictag_free_etherstubs(conf_stubs);

	nvlist_free(config);

	// get etherstubs (on the system)
	if (dladm_open(&handle) != DLADM_STATUS_OK)
		return 1;

	dladm_walk_datalink_id(walk_etherstub, handle, NULL,
	    DATALINK_CLASS_ETHERSTUB, DATALINK_ANY_MEDIATYPE, flags);

	if (handle != NULL)
		dladm_close(handle);

	return 0;
}

static int
do_update(int argc, char **argv)
{
	printf("do_update: argc = %d, argv[0] = %s\n", argc, argv[0]);
	return 0;
}

static int
do_vms(int argc, char **argv)
{
	printf("do_vms: argc = %d, argv[0] = %s\n", argc, argv[0]);
	return 0;
}

int main(int argc, char **argv) {
	int opt;
	char *subcmd;

	opts.opt_v = B_FALSE;
	while ((opt = getopt(argc, argv, "hv")) != -1) {
		switch (opt) {
		case 'h':
			usage(stdout);
			return (0);
		case 'v':
			opts.opt_v = B_TRUE;
			break;
		default:
			usage(stderr);
			return (1);
		}
	}

	argc -= optind;
	argv += optind;
	optind = 0;

	if (argc < 1)
		errx(1, "Command must be specified as first argument");

	subcmd = argv[0];
	argc--;
	argv++;

	if (strcmp(subcmd, "add") == 0)
		return do_add(argc, argv);
	else if (strcmp(subcmd, "delete") == 0)
		return do_delete(argc, argv);
	else if (strcmp(subcmd, "exists") == 0)
		return do_exists(argc, argv);
	else if (strcmp(subcmd, "list") == 0)
		return do_list(argc, argv);
	else if (strcmp(subcmd, "update") == 0)
		return do_update(argc, argv);
	else if (strcmp(subcmd, "vms") == 0)
		return do_vms(argc, argv);
	else
		errx(1, "Unknown command: %s", subcmd);

	return 0;
}
