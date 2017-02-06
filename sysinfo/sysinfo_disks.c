#include <err.h>
#include <limits.h>
#include <sys/utsname.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/dkio.h>
#include <sys/vtoc.h>
#include <fcntl.h>

#include <libnvpair.h>

void do_disk(const char *disk, nvlist_t *nvl) {
	int devnode;
	int ret;
	int removable;
	struct dk_minfo mediainfo;
	unsigned long long bytes;
	nvlist_t *sub_nvl;

	char devpath[PATH_MAX];
	snprintf(devpath, PATH_MAX, "/dev/rdsk/%sp0", disk);
	if ((devnode = open(devpath, O_RDONLY)) < 0 ) {
		warn("open(%s)", devpath);
		return;
	}

	ret = ioctl(devnode, DKIOCREMOVABLE, &removable);
	if (ret < 0) {
		warn("failed ioctl1");
		goto done;
	}
	if (removable) {
		// we don't care about these
		goto done;
	}

	ret = ioctl(devnode, DKIOCGMEDIAINFO, &mediainfo);
	if (ret < 0) {
		warn("failed ioctl2");
		goto done;
	}

	bytes = mediainfo.dki_capacity * mediainfo.dki_lbsize;

	if (nvlist_alloc(&sub_nvl, NV_UNIQUE_NAME, 0) != 0) {
		warn("nvlist_alloc");
		return;
	}

	fnvlist_add_int32(sub_nvl, "Size in GB", bytes / 1000 / 1000 / 1000);
	fnvlist_add_nvlist(nvl, disk, sub_nvl);
	nvlist_free(sub_nvl);

done:
	close(devnode);
}

void sysinfo_disks(nvlist_t *root_nvl) {
	DIR *d;
	struct dirent *dp;
	nvlist_t *nvl;

	d = opendir("/dev/dsk");
	if (d == NULL) {
		warn("opendir(/dev/dsk)");
		return;
	}

	if (nvlist_alloc(&nvl, NV_UNIQUE_NAME, 0) != 0) {
		warn("nvlist_alloc");
		closedir(d);
		return;
	}

	// loop over the dirent
	while ((dp = readdir(d)) != NULL) {
		// only interested in files that end in "s2"
		char *name = dp->d_name;
		int len = strlen(name);
		if (len < 2)
			continue;
		if (strncmp(name + (len - 2), "s2", 2) == 0) {
			// matches - chop off "s2"
			char name[PATH_MAX];
			strncpy(name, dp->d_name, len - 2);
			name[len - 2] = '\0';
			do_disk(name, nvl);
		}
	}

	closedir(d);

	fnvlist_add_nvlist(root_nvl, "Disks", nvl);
	nvlist_free(nvl);
}
