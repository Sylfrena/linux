#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/configfs.h>

#include "vkms_drv.h"
#include "vkms_drv.c"

/*
 * We already have something like this in vkms_drv.h. Use that maybe.
 * How tho
 */
struct vkms_configfs {
	struct configfs_subsystem subsys;
	int wb_item;

	int cursor;
};

//Returns vkms_config container for item
static inline struct vkms_configfs *to_vkms_configfs(struct config_item *item)
{
	return container_of(to_configfs_subsystem(to_config_group(item)), struct vkms_configfs, subsys);
}

struct wb {
	struct config_item item;
};

//static int unregister_vkm(struct vkms_configfs *vc)
//{
//	int ret;

//}

static ssize_t vkms_configfs_cursor_show(struct config_item *item, char *page)
{
	int cu = to_vkms_configfs(item)->cursor;

	return sprintf(page, "%d\n", cu);
}

static ssize_t vkms_configfs_wb_item_show(struct config_item *item, char *page)
{
	int cu = to_vkms_configfs(item)->wb_item;

	return sprintf(page, "%d\n", cu);
}

static ssize_t vkms_configfs_wb_item_store(struct config_item *item, const char *page,
		size_t count)
{
	struct vkms_configfs *vkms_configfs = to_vkms_configfs(item);
	int ret;

	ret = kstrtoint(page, 10, &vkms_configfs->wb_item);
	//str to int (page, base so 10 is base)
	
	if (ret)
		return ret;
	return count;
}


/*
 * pass attributes and tell machine these are attributes
 * I guess
 */

CONFIGFS_ATTR_RO(vkms_configfs_, cursor);
CONFIGFS_ATTR(vkms_configfs_, wb_item);

static struct configfs_attribute *vkms_configfs_attrs[] = {
	&vkms_configfs_attr_cursor,
	&vkms_configfs_attr_wb_item,
	NULL,
};

static const struct config_item_type vkms_configfs_type = {
	.ct_attrs = vkms_configfs_attrs,
	.ct_owner = THIS_MODULE,
};


//static struct configfs_item_operations vkms_config_item_ops = {
//	.release =
//}

static struct vkms_configfs vkms_configfs_subsys = {
	.subsys = {
		.su_group ={
			.cg_item = {
				.ci_namebuf = "00-vkms",
				.ci_type = &vkms_configfs_type,
			},
		},
	},
};

