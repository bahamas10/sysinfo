#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libnvpair.h>
#include <libdladm.h>

#define NICTAG_CONFIG "/usbkey/config"

/*
 * read the file "fname" and return a buffer on the heap
 * with the entire contents + a nul byte
 * must be free()d by caller
 */
static char *
read_file(const char *fname)
{
	long fsize;
	char *string;
	FILE *f;

	f = fopen(fname, "r");
	if (f == NULL)
		return NULL;

	fseek(f, 0, SEEK_END);
	fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	string = malloc(fsize + 1);
	if (string == NULL) {
		fclose(f);
		return NULL;
	}
	fread(string, fsize, 1, f);
	fclose(f);

	string[fsize] = '\0';
	return string;
}

static void
mac2str(const unsigned int *macaddr, char *buf)
{
	int i, n;
	n = sprintf(buf, "%02x", *macaddr++);
	for (i = 0; i < (ETHERADDRL - 1); i++)
		n += sprintf(buf+n, ":%02x", *macaddr++);
}

static boolean_t
str2mac(const char *buf, unsigned int *macaddr)
{
	return sscanf(buf, "%x:%x:%x:%x:%x:%x%c",
	    &macaddr[0],
	    &macaddr[1],
	    &macaddr[2],
	    &macaddr[3],
	    &macaddr[4],
	    &macaddr[5]) == 6 ? B_TRUE : B_FALSE;
}

char *
nictag_read_config()
{
	return read_file(NICTAG_CONFIG);
}

nvlist_t *
nictag_parse_config(char *buf)
{
	char *p;
	int index, offset;
	nvlist_t *nvl;

	if (nvlist_alloc(&nvl, NV_UNIQUE_NAME, 0) != 0)
		return NULL;

	// read file line-by-line
	index = 0;
	p = buf;
	while ((p = strchr(p, '\n')) != NULL) {
		int len;

		// arbitrarily sized
		char line[1024];
		char key[1024];
		char value[1024];

		boolean_t is_key;
		int i, j;

		offset = p - buf;
		len = offset - index;

		if (len >= sizeof (line)) {
			fprintf(stderr, "line is too long: %d characters - skipping\n", len);
			goto next;
		}

		// copy line from "buf" to "line"
		for (i = 0; i < len; i++)
			line[i] = buf[index + i];
		line[len] = '\0';

		// skip blanks and comments
		if (len == 0 || line[0] == '#')
			goto next;

		// parse out key + value
		is_key = B_TRUE;
		for (i = 0, j = 0; i < len; i++) {
			char c = line[i];
			if (c == '=') {
				is_key = B_FALSE;
				key[i] = '\0';
				continue;
			}
			if (is_key)
				key[i] = c;
			else
				value[j++] = c;
		}
		if (is_key) {
			// this means the line didn't have an = sign, bail
			fprintf(stderr, "bad line in config: '%s'\n", line);
			goto next;
		}
		value[j] = '\0';

		// store the data
		nvlist_add_string(nvl, key, value);
next:
		index = offset + 1;
		p++;
	}

	return nvl;
}

nvlist_t *
nictag_get_tags(nvlist_t *config)
{
	nvlist_t *nvl;
	nvpair_t *curr;

	if (nvlist_alloc(&nvl, NV_UNIQUE_NAME, 0) != 0)
		return NULL;

	for (curr = nvlist_next_nvpair(config, NULL); curr;
	    curr = nvlist_next_nvpair(config, curr)) {
		char tag[1024];
		char *mac_str;
		char mac_normalized[1024];
		unsigned int mac_num[ETHERADDRL];
		char *name = nvpair_name(curr);
		size_t namelen = strlen(name);

		// look for <tag>_nic
		if (strncmp(name + (namelen - 4), "_nic", 4) != 0)
			continue;
		snprintf(tag, namelen - 3, "%s", name);

		// extract the value (which should be a mac addr) and normalize
		// it.  we normalize by converting the string to a mac address
		// number, and back to a string - this also acts as validation
		nvpair_value_string(curr, &mac_str);
		if (!str2mac(mac_str, mac_num)) {
			fprintf(stderr, "failed to parse mac: %s\n", mac_str);
			continue;
		}
		mac2str(mac_num, mac_normalized);
		nvlist_add_string(nvl, tag, mac_normalized);
	}

	return nvl;
}

char **
nictag_get_etherstubs(nvlist_t *config)
{
	char *stubline;
	char **etherstubs;
	char *comma, *position;
	char stubname[1024];
	int numstub = 0, len;
	char *s;

	if (nvlist_lookup_string(config, "etherstub", &stubline) != 0)
		return NULL;

	etherstubs = malloc(sizeof (char *) * 1024);
	if (etherstubs == NULL)
		return NULL;

	comma = strchr(stubline, ',');
	position = stubline;
	while (comma != NULL) {
		int i = 0;
		while (position < comma) {
			stubname[i] = *position;
			i++;
			position++;
		}

		stubname[i] = '\0';
		s = malloc(i + 1);
		strncpy(s, stubname, i);
		etherstubs[numstub++] = s;

		position++;
		comma = strchr(position, ',');
	}
	len = strlen(position);
	s = malloc(len + 1);
	strncpy(s, position, len);
	etherstubs[numstub++] = s;
	etherstubs[numstub++] = NULL;

	return etherstubs;
}

void
nictag_free_etherstubs(char **etherstubs)
{
	char *stub;
	if (etherstubs == NULL)
		return;
	while ((stub = *etherstubs++) != NULL)
		free(stub);
	free(etherstubs);
}
