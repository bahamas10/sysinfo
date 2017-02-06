/*
 * Copyright 2015 Joyent Inc.
 */

#include <err.h>
#include <fcntl.h>
#include <libdevinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <libnvpair.h>

/*
 * In this comment typed properties are those of type DI_PROP_TYPE_UNDEF_IT,
 * DI_PROP_TYPE_BOOLEAN, DI_PROP_TYPE_INT, DI_PROP_TYPE_INT64,
 * DI_PROP_TYPE_BYTE, and DI_PROP_TYPE_STRING.
 *
 * The guessing algorithm is:
 * 1. If the property is typed and the type is consistent with the value of
 *    the property, then the property is of that type. If the type is not
 *    consistent with value of the property, then the type is treated as
 *    alien to prtconf.
 * 2. If the property is of type DI_PROP_TYPE_UNKNOWN the following steps
 *    are carried out.
 *    a. If the value of the property is consistent with a string property,
 *       the type of the property is DI_PROP_TYPE_STRING.
 *    b. Otherwise, if the value of the property is consistent with an integer
 *       property, the type of the property is DI_PROP_TYPE_INT.
 *    c. Otherwise, the property type is treated as alien to prtconf.
 * 3. If the property type is alien to prtconf, then the property value is
 *    read by the appropriate routine for untyped properties and the following
 *    steps are carried out.
 *    a. If the length that the property routine returned is zero, the
 *       property is of type DI_PROP_TYPE_BOOLEAN.
 *    b. Otherwise, if the length that the property routine returned is
 *       positive, then the property value is treated as raw data of type
 *       DI_PROP_TYPE_UNKNOWN.
 *    c. Otherwise, if the length that the property routine returned is
 *       negative, then there is some internal inconsistency and this is
 *       treated as an error and no type is determined.
 *
 *
 * Joyent/jwilsdon: This function was taken and modified from:
 *
 *     <illumos>/usr/src/cmd/prtconf/pdevinfo.c
 *
 */
static int prop_type_guess(di_prop_t prop, void **prop_data, int *prop_type) {
    int len, type;

    type = di_prop_type(prop);
    switch (type) {
    case DI_PROP_TYPE_UNDEF_IT:
    case DI_PROP_TYPE_BOOLEAN:
        *prop_data = NULL;
        *prop_type = type;
        return (0);
    case DI_PROP_TYPE_INT:
        len = di_prop_ints(prop, (int **)prop_data);
        break;
    case DI_PROP_TYPE_INT64:
        len = di_prop_int64(prop, (int64_t **)prop_data);
        break;
    case DI_PROP_TYPE_BYTE:
        len = di_prop_bytes(prop, (uchar_t **)prop_data);
        break;
    case DI_PROP_TYPE_STRING:
        len = di_prop_strings(prop, (char **)prop_data);
        break;
    case DI_PROP_TYPE_UNKNOWN:
        len = di_prop_strings(prop, (char **)prop_data);
        if ((len > 0) && ((*(char **)prop_data)[0] != 0)) {
            *prop_type = DI_PROP_TYPE_STRING;
            return (len);
        }

        len = di_prop_ints(prop, (int **)prop_data);
        type = DI_PROP_TYPE_INT;

        break;
    default:
        len = -1;
    }

    if (len > 0) {
        *prop_type = type;
        return (len);
    }

    len = di_prop_rawdata(prop, (uchar_t **)prop_data);
    if (len < 0) {
        return (-1);
    } else if (len == 0) {
        *prop_type = DI_PROP_TYPE_BOOLEAN;
        return (0);
    }

    *prop_type = DI_PROP_TYPE_UNKNOWN;
    return (len);
}

static void do_prop(di_prop_t prop, nvlist_t *nvl) {
	int prop_type, nitems;
	char *name;
	void *prop_data;

	nitems = prop_type_guess(prop, &prop_data, &prop_type);

	/* XXX: currently we only handle single string properties because those are
	 *      all that are needed for showing boot parameters.
	 */
	if ((nitems != 1) || (prop_type != DI_PROP_TYPE_STRING))
		return;

	name = di_prop_name(prop);
	fnvlist_add_string(nvl, name, (char *)prop_data);
}

static int do_node(di_node_t node, void *arg) {
	nvlist_t *nvl = (nvlist_t *)arg;
	di_prop_t prop = DI_PROP_NIL;

	if (strcmp(di_node_name(node), "i86pc") == 0) {
		while ((prop = di_prop_next(node, prop)) != DI_PROP_NIL) {
			do_prop(prop, nvl);
		}
		return (DI_WALK_TERMINATE);
	}
	return (DI_WALK_CONTINUE);
}

void sysinfo_bootparams(nvlist_t *root_nvl) {
	nvlist_t *nvl;
	di_node_t root_node;

	if (nvlist_alloc(&nvl, NV_UNIQUE_NAME, 0) != 0) {
		warn("nvlist_alloc");
		return;
	}

	root_node = di_init("/", (DINFOSUBTREE | DINFOPROP));
	if (root_node == DI_NODE_NIL) {
		warn("di_init() failed\n");
		return;
	}

	di_walk_node(root_node, DI_WALK_CLDFIRST, nvl, do_node);
	di_fini(root_node);

	fnvlist_add_nvlist(root_nvl, "Boot Parameters", nvl);
	nvlist_free(nvl);
}
