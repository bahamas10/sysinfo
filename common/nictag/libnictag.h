#ifndef libnictag_h__
#define libnictag_h__

#include <libnvpair.h>

/*
 * read the nictag config and return a nul terminated
 * text buffer. returns NULL on error
 */
char *nictag_read_config(void);

/*
 * given a text buffer of the USB config file, return an nvlist
 * with each key=>value pair extracted (all as strings)
 * must bee free()d by caller
 */
nvlist_t *nictag_parse_config(char *buf);

/*
 * given an nvlist created from nictac_parse_config, return a new
 * nvlist that maps nic tag name to mac address, ie:
 * {
 *   "admin": "00:11:22:aa:bb:cc",
 *   "external": "33:44:55:dd:ee:ff"
 * }
 * must be free()d by caller
 */
nvlist_t *nictag_get_tags(nvlist_t *config);

/*
 * given an nvlist created from nictac_parse_config, return a
 * list of strings for each etherstub found, terminated by a NULL ptr.
 * must be free()d by caller
 */
char **nictag_get_etherstubs(nvlist_t *config);

/*
 * free a list given by nictag_get_etherstubs
 */
void nictag_free_etherstubs(char **etherstubs);

#endif // libnictag_h__
