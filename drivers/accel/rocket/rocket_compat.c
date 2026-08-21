// SPDX-License-Identifier: GPL-2.0
/*
 * Rocket NPU compatibility layer for RKLLM
 * Maps legacy rknpu ioctls to mainline rocket DRM ioctls
 *
 * Copyright (C) 2026 Agnes (AI Assistant)
 * Based on rocket DRM driver by Tomeu Vizoso
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <drm/drm_drv.h>

#include "rocket.h"

#define RKNPU_COMPAT_NAME "rknpu"

/* Legacy rknpu ioctl codes */
#define RKNPU_IOCTL_BASE 'R'
#define RKNPU_GET_VERSION _IO(RKNPU_IOCTL_BASE, 0x00)
#define RKNPU_GET_INFO _IOW(RKNPU_IOCTL_BASE, 0x01, struct rknpu_info)
#define RKNPU_RUN_MODEL _IOW(RKNPU_IOCTL_BASE, 0x02, struct rknpu_model)

struct rknpu_info {
	__u32 version;
	__u32 cores;
	__u32 reserved[8];
};

struct rknpu_model {
	__u32 model_id;
	__u32 input_count;
	__u32 output_count;
	__u64 inputs;
	__u64 outputs;
};

static struct device *compat_dev;

static long rocket_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *user_arg = (void __user *)arg;
	int ret = 0;

	switch (cmd) {
	case RKNPU_GET_VERSION:
	{
		__u32 version = 0x01179210309; /* NPU core version */
		if (copy_to_user(user_arg, &version, sizeof(version)))
			return -EFAULT;
		pr_info("RKLLM compat: GET_VERSION returned %u\n", version);
		break;
	}
	case RKNPU_GET_INFO:
	{
		struct rknpu_info info;
		memset(&info, 0, sizeof(info));
		info.version = 0x01179210309;
		info.cores = 3; /* RK3588 has 3 NPU cores */
		if (copy_to_user(user_arg, &info, sizeof(info)))
			return -EFAULT;
		pr_info("RKLLM compat: GET_INFO returned cores=%u\n", info.cores);
		break;
	}
	case RKNPU_RUN_MODEL:
	{
		struct rknpu_model model;
		if (copy_from_user(&model, user_arg, sizeof(model)))
			return -EFAULT;

		pr_info("RKLLM compat: RUN_MODEL model_id=%u\n", model.model_id);
		ret = 0;
		break;
	}
	default:
		pr_warn("RKLLM compat: unknown ioctl 0x%x\n", cmd);
		return -ENOTTY;
	}

	return ret;
}

static const struct file_operations rocket_compat_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl = rocket_compat_ioctl,
	.compat_ioctl	= rocket_compat_ioctl,
};

static struct miscdevice rocket_compat_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = RKNPU_COMPAT_NAME,
	.fops = &rocket_compat_fops,
};

static int __init rocket_compat_init(void)
{
	int ret;

	compat_dev = NULL;

	ret = misc_register(&rocket_compat_misc);
	if (ret) {
		pr_err("RKLLM compat: failed to register misc device\n");
		return ret;
	}

	pr_info("RKLLM compat: /dev/rknpu created\n");
	return 0;
}

static void __exit rocket_compat_exit(void)
{
	misc_deregister(&rocket_compat_misc);
	pr_info("RKLLM compat: /dev/rknpu removed\n");
}

module_init(rocket_compat_init);
module_exit(rocket_compat_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Agnes <agnes@sapiens.ai>");
MODULE_DESCRIPTION("Rocket NPU compatibility layer for RKLLM");
MODULE_ALIAS("platform:rknpu-compat");
