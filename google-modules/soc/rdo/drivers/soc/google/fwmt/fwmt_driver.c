// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2025 Google LLC.
 *
 */

#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <soc/google/goog_gdmc_service_ids.h>
#include <soc/google/goog-mba-gdmc-iface.h>
#include <soc/google/goog_mba_nq_xport.h>

#include "fwmt_driver.h"
#include "fwmt_ipc.h"
#include "fwmt_service.h"

#define DEVICE_NUMBER 2

static int fwmt_cdev_open(struct inode *inodep, struct file *filep)
{
	struct fwmt_base *base =
		container_of(inodep->i_cdev, struct fwmt_cdev_instance, cdev)
			->base;
	struct cdev_state *cdev_state;
	enum fwmt_mba_op_type op_type;

	op_type = MINOR(inodep->i_rdev);
	if (op_type > DEVICE_NUMBER) {
		dev_err(base->dev, "Invalid minor number %d\n", op_type);
		return -EFAULT;
	}

	cdev_state = get_cdev_state(base, op_type);
	if (IS_ERR(cdev_state)) {
		dev_err(base->dev, "Could not initialize device state\n");
		return PTR_ERR(cdev_state);
	}

	filep->private_data = cdev_state;
	return 0;
}

static ssize_t fwmt_cdev_read(struct file *filep, char __user *buffer,
			      size_t len, loff_t *offset)
{
	struct cdev_state *cdev_state = filep->private_data;
	uint32_t bytes_written;
	ssize_t ret;

	if (*offset >= cdev_state->total_size)
		return 0;

	mutex_lock(&cdev_state->base->buffer_lock);

	ret = refill_mba_buffer(cdev_state, len, *offset, &bytes_written);
	if (ret)
		goto out;

	if (copy_to_user(buffer, cdev_state->mbi.mba_buffer, bytes_written)) {
		dev_err(cdev_state->base->dev, "Failed to copy data to user\n");
		ret = -EFAULT;
		goto out;
	}

	*offset += bytes_written;
	ret = bytes_written;

out:
	mutex_unlock(&cdev_state->base->buffer_lock);
	return ret;
}

static int fwmt_cdev_release(struct inode *inodep, struct file *filep)
{
	put_cdev_state(filep->private_data);
	return 0;
}

static struct file_operations fwmt_fops = {
	.open = fwmt_cdev_open,
	.read = fwmt_cdev_read,
	.release = fwmt_cdev_release,
	.owner = THIS_MODULE,
};

static int create_chardev_instance(struct fwmt_base *base,
				   struct fwmt_cdev_instance *cdevi, dev_t devt,
				   const char *name)
{
	struct device *cdev_node;
	int ret;

	cdev_init(&cdevi->cdev, &fwmt_fops);
	cdevi->cdev.owner = THIS_MODULE;
	cdevi->devt = devt;
	cdevi->base = base;

	ret = cdev_add(&cdevi->cdev, cdevi->devt, 1);
	if (ret) {
		dev_err(base->dev, "Failed to add cdev for %s\n", name);
		return ret;
	}

	cdev_node = device_create(base->class, base->dev, cdevi->devt, NULL,
				  "%s", name);
	if (IS_ERR(cdev_node)) {
		dev_err(base->dev, "Failed to create device file for %s\n",
			name);
		ret = PTR_ERR(cdev_node);
		cdev_del(&cdevi->cdev);
		return ret;
	}

	return 0;
}

static void destroy_chardev_instance(struct fwmt_base *base,
				     struct fwmt_cdev_instance *cdevi)
{
	device_destroy(base->class, cdevi->devt);
	cdev_del(&cdevi->cdev);
}

static int fwmt_init_chardev(struct fwmt_base *base)
{
	int ret;

	ret = alloc_chrdev_region(&base->devt, 0, DEVICE_NUMBER,
				  KBUILD_MODNAME);
	if (ret) {
		dev_err(base->dev, "Failed to allocate a major number.\n");
		goto err_alloc_chrdev;
	}

	base->class = class_create(KBUILD_MODNAME);
	if (IS_ERR(base->class)) {
		dev_err(base->dev, "Failed to create device class.\n");
		ret = PTR_ERR(base->class);
		goto err_class_create;
	}

	ret = create_chardev_instance(base, &base->cdev_strings,
				      MKDEV(MAJOR(base->devt),
					    GDMC_MBA_FWMT_RETRIEVE_STRING),
				      KBUILD_MODNAME "_strings");
	if (ret)
		goto err_create_strings;

	ret = create_chardev_instance(base, &base->cdev_metrics,
				      MKDEV(MAJOR(base->devt),
					    GDMC_MBA_FWMT_RETRIEVE_METRIC),
				      KBUILD_MODNAME "_metrics");
	if (ret)
		goto err_create_metrics;

	return 0;

err_create_metrics:
	destroy_chardev_instance(base, &base->cdev_strings);
err_create_strings:
	class_destroy(base->class);
err_class_create:
	unregister_chrdev_region(base->devt, DEVICE_NUMBER);
err_alloc_chrdev:
	return ret;
}

static void fwmt_destroy_chardev(struct fwmt_base *base)
{
	destroy_chardev_instance(base, &base->cdev_metrics);
	destroy_chardev_instance(base, &base->cdev_strings);
	class_destroy(base->class);
	unregister_chrdev_region(base->devt, 2);
}

static int fwmt_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fwmt_base *base;
	struct device_node *rmem_node;
	struct reserved_mem *rmem;
	u32 offset = 0;
	u32 size = 0;
	int ret;

	base = devm_kzalloc(dev, sizeof(struct fwmt_base), GFP_KERNEL);
	if (!base)
		return -ENOMEM;

	mutex_init(&base->buffer_lock);

	platform_set_drvdata(pdev, base);
	base->dev = dev;
	base->gdmc_iface = gdmc_iface_get(dev);

	if (IS_ERR(base->gdmc_iface))
		return PTR_ERR(base->gdmc_iface);

	rmem_node = of_parse_phandle(dev->of_node, "memory-region", 0);
	if (!rmem_node) {
		dev_err(dev, "Failed to parse memory-region phandle\n");
		ret = -ENODEV;
		goto err_gdmc_put;
	}

	rmem = of_reserved_mem_lookup(rmem_node);
	of_node_put(rmem_node);
	if (!rmem) {
		dev_err(dev, "Failed to lookup reserved memory\n");
		ret = -ENODEV;
		goto err_gdmc_put;
	}

	ret = of_property_read_u32(dev->of_node, "buffer-offset", &offset);
	if (ret) {
		dev_err(dev, "Failed to read buffer-offset property\n");
		goto err_gdmc_put;
	}

	ret = of_property_read_u32(dev->of_node, "buffer-size", &size);
	if (ret) {
		dev_err(dev, "Failed to read buffer-size property\n");
		goto err_gdmc_put;
	}

	if (offset + size > rmem->size) {
		dev_err(dev, "Offset and size exceed reserved memory bounds\n");
		ret = -EINVAL;
		goto err_gdmc_put;
	}

	if (size < sizeof(struct fwmt_msg_request_buffer)) {
		dev_err(dev, "Buffer size too small\n");
		ret = -EINVAL;
		goto err_gdmc_put;
	}

	base->mba_buffer_size = size;
	base->mba_buffer_paddr = rmem->base + offset;

	base->mba_buffer =
		devm_memremap(dev, base->mba_buffer_paddr, size, MEMREMAP_WC);
	if (!base->mba_buffer) {
		dev_err(dev, "Failed to map reserved memory\n");
		ret = -ENOMEM;
		goto err_gdmc_put;
	}

	ret = fwmt_init_chardev(base);
	if (ret)
		goto err_gdmc_put;

	return 0;

err_gdmc_put:
	gdmc_iface_put(base->gdmc_iface);
	return ret;
}

static void fwmt_remove(struct platform_device *pdev)
{
	struct fwmt_base *base = platform_get_drvdata(pdev);

	fwmt_destroy_chardev(base);
	gdmc_iface_put(base->gdmc_iface);
}

static const struct of_device_id fwmt_of_match[] = {
	{ .compatible = "google,fwmt-gdmc" },
	{},
};
MODULE_DEVICE_TABLE(of, fwmt_of_match);

static struct platform_driver fwmt_platform_driver = {
	.probe = fwmt_probe,
	.remove = fwmt_remove,
	.driver = {
		.name = KBUILD_MODNAME,
		.of_match_table = fwmt_of_match,
	},
};
module_platform_driver(fwmt_platform_driver);

MODULE_AUTHOR("Filip Konieczny <filipkonieczny@google.com>");
MODULE_DESCRIPTION("GDMC Firmware Metrics");
MODULE_LICENSE("GPL");
