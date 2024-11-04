#pragma once

#include <proc/vfs.h>

struct tmpfs_handle
{
	struct handle handle;
	void* buffer;
	size_t buffer_cap;
};

bool tmpfs_init();