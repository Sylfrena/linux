// SPDX-License-Identifier: GPL-2.0+

#include "vkms_drv.h"

static struct config_group *connector_group;
static struct vkms_device *vkms_device;

static char *available_connectors[] = {
	"Virtual",
	"Writeback",
};

struct connectors_fs {
	struct config_item item;
	bool enable;
};

static int enable_connector(const char *name)
{
	int ret = 0;

	if (!strcmp(name, "Writeback"))
		ret = vkms_enable_writeback_connector(vkms_device);
	else if (!strcmp(name, "Virtual"))
		ret = enable_virtual_connector(vkms_device);

	if (ret)
		return ret;

	drm_mode_config_reset(&vkms_device->drm);
	return 0;
}

static void disable_connector(const char *name)
{
	if (!strcmp(name, "Writeback"))
		disable_writeback_connector(vkms_device);
	else if (!strcmp(name, "Virtual"))
		disable_virtual_connector(vkms_device);
}

static inline struct connectors_fs *to_conn_item(struct config_item *item)
{
	return item ? container_of(item, struct connectors_fs, item) : NULL;
}

static ssize_t conn_fs_enable_show(struct config_item *item, char *page)
{
	struct connectors_fs *conn_item = to_conn_item(item);

	return sprintf(page, "%d\n", conn_item->enable);
}

static ssize_t conn_fs_enable_store(struct config_item *item,
				    const char *page, size_t count)
{
	struct connectors_fs *conn_item = to_conn_item(item);
	char *p = (char *)page;
	unsigned int enable;
	int ret;

	ret = kstrtouint(p, 10, &enable);
	if (ret)
		return -EINVAL;

	if (enable > 1)
		return -EINVAL;

	if (enable == conn_item->enable)
		return count;

	if (enable) {
		ret = enable_connector(item->ci_name);
		if (ret)
			return ret;
	} else {
		disable_connector(item->ci_name);
	}

	conn_item->enable = enable ? true : false;
	return count;
}

CONFIGFS_ATTR(conn_fs_, enable);

static struct configfs_attribute *connectors_fs_attrs[] = {
	&conn_fs_attr_enable,
	NULL,
};

static void connectors_fs_release(struct config_item *item)
{
	kfree(to_conn_item(item));
}

static struct configfs_item_operations connectors_fs_ops = {
	.release = connectors_fs_release,
};

static const struct config_item_type connectors_fs_type = {
	.ct_item_ops	= &connectors_fs_ops,
	.ct_attrs	= connectors_fs_attrs,
	.ct_owner	= THIS_MODULE,
};

static struct config_item *make_connector_item(struct config_group *group,
					       const char *name)
{
	struct connectors_fs *conn_item;
	int i, ret, total_conn = ARRAY_SIZE(available_connectors);

	for (i = 0; i < total_conn; i++)
		if (!strcmp(name, available_connectors[i]))
			break;

	if (i == total_conn)
		return ERR_PTR(-EINVAL);

	ret = enable_connector(name);
	if (ret)
		return ERR_PTR(ret);

	conn_item = kzalloc(sizeof(*conn_item), GFP_KERNEL);
	if (!conn_item)
		return ERR_PTR(-ENOMEM);

	config_item_init_type_name(&conn_item->item, name,
				   &connectors_fs_type);

	conn_item->enable = true;

	return &conn_item->item;
}

static void drop_connector_item(struct config_group *group,
				struct config_item *item)
{
	char *name = item->ci_name;

	disable_connector(name);

	config_item_put(item);
}

static struct configfs_group_operations connector_group_ops = {
	.make_item	= make_connector_item,
	.drop_item	= drop_connector_item,
};

static const struct config_item_type connector_type = {
	.ct_group_ops	= &connector_group_ops,
	.ct_owner	= THIS_MODULE,
};

static const struct config_item_type vkms_subsystem_type = {
	.ct_owner	= THIS_MODULE,
};

static struct configfs_subsystem vkms_subsystem = {
	.su_group = {
		.cg_item = {
			.ci_namebuf = "vkms",
			.ci_type = &vkms_subsystem_type,
		},
	},
	.su_mutex = __MUTEX_INITIALIZER(vkms_subsystem.su_mutex),
};

static void init_default_conn_configfs(struct config_group *root)
{
	struct vkms_config_state *config_state = &vkms_device->config_state;
	int i, ret;

	connector_group = configfs_register_default_group(root, "connectors",
							  &connector_type);
	if (IS_ERR(connector_group)) {
		ret = PTR_ERR(connector_group);
		pr_err("Error %d while registering functions group\n", ret);
		return;
	}

	for (i = 0; i < ARRAY_SIZE(available_connectors); i++) {
		struct config_group *group = config_state->connectors[i];

		//if (!strcmp(available_connectors[i], "Writeback") &&
		 //   !enable_writeback)
		//	continue;

		group = configfs_register_default_group(connector_group,
							available_connectors[i],
							&connectors_fs_type);
		if (IS_ERR(connector_group)) {
			ret = PTR_ERR(config_state->connectors[i]);
			DRM_ERROR("Error %d while trying to register %s\n",
				  ret, available_connectors[i]);
			continue;
		}

		to_conn_item(&group->cg_item)->enable = true;
	}
}

int vkms_configfs_init(struct vkms_device *vkmsdev)
{
	struct config_group *root = &vkms_subsystem.su_group;
	int ret;

	vkms_device = vkmsdev;

	config_group_init(root);
	ret = configfs_register_subsystem(&vkms_subsystem);
	if (ret) {
		pr_err("Error %d while registering subsystem %s\n", ret,
		       root->cg_item.ci_namebuf);
		goto err;
	}

	init_default_conn_configfs(root);

	return 0;

err:
	return ret;
}

void vkms_configfs_exit(void)
{
	configfs_unregister_subsystem(&vkms_subsystem);
}
