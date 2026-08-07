/* isoscan.c - Optimized ISO scanner for GRUB 2.14 by Ali ELÇİ */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2025  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/disk.h>
#include <grub/device.h>
#include <grub/file.h>
#include <grub/fs.h>
#include <grub/partition.h>
#include <grub/extcmd.h>
#include <grub/normal.h>
#include <grub/i18n.h>
#include <grub/env.h>
#include <grub/command.h>
#include <grub/term.h>

GRUB_MOD_LICENSE ("GPLv3+");

#define ISOSCAN_MAX_DEPTH    3
#define ISOSCAN_MAX_ENTRIES  64
#define ISOSCAN_MAX_NODES    4000
#define ISOSCAN_PATHBUF_SIZE 1024

struct ext_entry {
    const char *ext;
    grub_uint8_t len;
};

static const struct ext_entry isoscan_extensions[] =
  {
    {".iso", 4},
    {".img", 4},
    {".ima", 4},
    {".vhd", 4},
    {".vhdx", 5},
    {".wim", 4},
    {NULL, 0}
  };

static const char *isoscan_skip_dirs[] =
  {
    "windows",
    "program files",
    "program files (x86)",
    "programdata",
    "appdata",
    "$recycle.bin",
    "system volume information",
    "node_modules",
    ".git",
    "windowsapps",
    "boot",
    "recovery",
    "perflogs",
    "msocache",
    "$winreagent",
    "$windows.~bt",
    "$windows.~ws",
    "efi",
    "proc",
    "sys",
    "dev",
    "run",
    "tmp",
    "lost+found",
    "snap",
    NULL
  };

static const struct grub_arg_option options[] =
  {
    {"root-only", 'r', 0,
     N_("Only scan the root directory of each filesystem (fast mode)."),
     0, 0},
    {"path", 'p', 0,
     N_("Restrict scan to the given sub-path on every device."),
     N_("PATH"), ARG_TYPE_STRING},
    {"quiet", 'q', 0,
     N_("Suppress all output messages."),
     0, 0},
    {"progress", 'P', 0,
     N_("Show scan progress."),
     0, 0},
    {"all-images", 'a', 0,
     N_("Also scan .img, .ima, .vhd, .vhdx, .wim files."),
     0, 0},
    {0, 0, 0, 0, 0, 0}
  };

enum
  {
    ISOSCAN_ROOT_ONLY,
    ISOSCAN_PATH,
    ISOSCAN_QUIET,
    ISOSCAN_PROGRESS,
    ISOSCAN_ALL_IMAGES
  };

struct isoscan_ctx
{
    int root_only;
    int quiet;
    int progress;
    int all_images;
    const char *restrict_path;
    int found;
    int depth;
    const char *devname;
    grub_device_t device;
    grub_fs_t fs;
    int visited;
    char pathbuf[ISOSCAN_PATHBUF_SIZE];
};

/* ================================================================
   FORWARD DECLARATIONS (Tüm Prototipler En Üstte)
   ================================================================ */
static const char *isoscan_basename (const char *path);
static char *isoscan_build_source (const char *devname, const char *isopath);
static grub_err_t isoscan_add_entry (const char *devname, const char *isopath);
static int isoscan_has_extension (const char *filename, int all_images);
static int isoscan_should_skip_dir (const char *name);
static void isoscan_scan_dir (struct isoscan_ctx *ctx, const char *path);
static int isoscan_dir_hook (const char *filename, const struct grub_dirhook_info *info, void *data);
static int isoscan_device_hook (const char *name, void *data);

/* ================================================================
   UTILITY FUNCTIONS
   ================================================================ */
static const char *
isoscan_basename (const char *path)
{
    const char *p = path;
    const char *last = path;

    while (*p)
    {
        if (*p == '/')
            last = p + 1;
        p++;
    }

    return last;
}

static int
isoscan_has_extension (const char *filename, int all_images)
{
    grub_size_t len = grub_strlen (filename);
    int i;

    if (len < 4)
        return 0;

    if (grub_strcasecmp (filename + len - 4, ".iso") == 0)
        return 1;

    if (!all_images)
        return 0;

    for (i = 0; isoscan_extensions[i].ext; i++)
    {
        grub_size_t ext_len = isoscan_extensions[i].len;
        if (len >= ext_len &&
            grub_strcasecmp (filename + len - ext_len, 
                             isoscan_extensions[i].ext) == 0)
            return 1;
    }

    return 0;
}

static int
isoscan_should_skip_dir (const char *name)
{
    int i;

    for (i = 0; isoscan_skip_dirs[i]; i++)
        if (grub_strcasecmp (name, isoscan_skip_dirs[i]) == 0)
            return 1;

    return 0;
}


static char *
isoscan_build_source (const char *devname, const char *isopath)
{
    return grub_xasprintf (
        "set isoscan_target=\"(%s)%s\" ; "
        "configfile $prefix/grub.cfg",
        devname, isopath);
}

static grub_err_t
isoscan_add_entry (const char *devname, const char *isopath)
{
    char *title;
    char *source;
    const char *args[2];
    const char *classes_storage[4];
    const char *basename;
    char *display_name;
    grub_size_t len;
    grub_err_t err;

    basename = isoscan_basename (isopath);
    
    display_name = grub_strdup (basename);
    if (!display_name)
        return grub_errno ? : GRUB_ERR_OUT_OF_MEMORY;
    
    len = grub_strlen (display_name);
    if (len > 4 && grub_strcasecmp (display_name + len - 4, ".iso") == 0)
        display_name[len - 4] = '\0';
    else if (len > 4 && grub_strcasecmp (display_name + len - 4, ".img") == 0)
        display_name[len - 4] = '\0';
    else if (len > 4 && grub_strcasecmp (display_name + len - 4, ".vhd") == 0)
        display_name[len - 4] = '\0';
    else if (len > 5 && grub_strcasecmp (display_name + len - 5, ".vhdx") == 0)
        display_name[len - 5] = '\0';
    else if (len > 4 && grub_strcasecmp (display_name + len - 4, ".wim") == 0)
        display_name[len - 4] = '\0';

    title = grub_xasprintf ("[Wboot] %s", display_name);
    if (!title)
    {
        grub_free (display_name);
        return grub_errno ? : GRUB_ERR_OUT_OF_MEMORY;
    }

    source = isoscan_build_source (devname, isopath);
    if (!source)
    {
        grub_free (display_name);
        grub_free (title);
        return grub_errno ? : GRUB_ERR_OUT_OF_MEMORY;
    }

    args[0] = title;
    args[1] = NULL;

    classes_storage[0] = "isoscan";
    classes_storage[1] = "wboot";
    classes_storage[2] = "iso";
    classes_storage[3] = NULL;

    err = grub_normal_add_menu_entry (1, args, (char **) classes_storage, 
                                       NULL, NULL, NULL, NULL, source, 0, NULL);

    grub_free (display_name);
    grub_free (title);
    grub_free (source);

    return err;
}

/* ================================================================
   SCANNING ENGINE
   ================================================================ */
static void
isoscan_scan_dir (struct isoscan_ctx *ctx, const char *path)
{
    if (ctx->found >= ISOSCAN_MAX_ENTRIES)
        return;

    if (ctx->visited >= ISOSCAN_MAX_NODES)
        return;

    if (!ctx->fs || !ctx->fs->fs_dir)
        return;

    ctx->fs->fs_dir (ctx->device, path, isoscan_dir_hook, ctx);
    if (grub_errno)
        grub_errno = GRUB_ERR_NONE;
}

static int
isoscan_dir_hook (const char *filename, const struct grub_dirhook_info *info,
                   void *data)
{
    struct isoscan_ctx *ctx = data;
    grub_size_t path_len;
    grub_size_t file_len;
    grub_size_t new_len;
    char saved_char;

    if (filename[0] == '.')
        return 0;

    if (ctx->found >= ISOSCAN_MAX_ENTRIES)
        return 1;

    ctx->visited++;
    if (ctx->visited >= ISOSCAN_MAX_NODES)
        return 1;

    path_len = grub_strlen (ctx->pathbuf);
    file_len = grub_strlen (filename);

    if (info->dir)
    {
        if (ctx->root_only)
            return 0;
        if (ctx->depth >= ISOSCAN_MAX_DEPTH)
            return 0;
        if (isoscan_should_skip_dir (filename))
            return 0;

        new_len = path_len + file_len + 2;
        if (new_len >= ISOSCAN_PATHBUF_SIZE)
            return 0;

        saved_char = ctx->pathbuf[path_len];
        grub_memcpy (ctx->pathbuf + path_len, filename, file_len);
        ctx->pathbuf[path_len + file_len] = '/';
        ctx->pathbuf[path_len + file_len + 1] = '\0';

        ctx->depth++;
        isoscan_scan_dir (ctx, ctx->pathbuf);
        ctx->depth--;

        ctx->pathbuf[path_len] = saved_char;

        return 0;
    }

    if (!isoscan_has_extension (filename, ctx->all_images))
        return 0;

    new_len = path_len + file_len + 1;
    if (new_len >= ISOSCAN_PATHBUF_SIZE)
        return 0;

    saved_char = ctx->pathbuf[path_len];
    grub_memcpy (ctx->pathbuf + path_len, filename, file_len);
    ctx->pathbuf[path_len + file_len] = '\0';

    if (isoscan_add_entry (ctx->devname, ctx->pathbuf) == GRUB_ERR_NONE)
    {
        ctx->found++;
        if (ctx->progress && !ctx->quiet)
            grub_printf ("  ✓ [%s] %s\n", ctx->devname, ctx->pathbuf);
    }
    else
    {
        if (!ctx->quiet)
            grub_printf ("  ✗ Failed: %s%s\n", ctx->devname, ctx->pathbuf);
        if (grub_errno)
            grub_errno = GRUB_ERR_NONE;
    }

    ctx->pathbuf[path_len] = saved_char;

    return 0;
}

static int
isoscan_device_hook (const char *name, void *data)
{
    struct isoscan_ctx *ctx = data;
    grub_device_t dev;
    grub_size_t path_len;

    if (ctx->found >= ISOSCAN_MAX_ENTRIES)
        return 1;

    dev = grub_device_open (name);
    if (!dev)
    {
        if (grub_errno)
            grub_errno = GRUB_ERR_NONE;
        return 0;
    }

    if (!dev->disk)
    {
        grub_device_close (dev);
        return 0;
    }

    if (!grub_strchr (name, ','))
    {
        grub_device_close (dev);
        return 0;
    }

    ctx->fs = grub_fs_probe (dev);
    if (!ctx->fs)
    {
        if (grub_errno)
            grub_errno = GRUB_ERR_NONE;
        grub_device_close (dev);
        return 0;
    }

    ctx->devname = name;
    ctx->device = dev;
    ctx->depth = 0;

    if (ctx->restrict_path && ctx->restrict_path[0] != '\0')
    {
        path_len = grub_strlen (ctx->restrict_path);
        if (path_len + 2 >= ISOSCAN_PATHBUF_SIZE)
        {
            grub_device_close (dev);
            return 0;
        }

        if (ctx->restrict_path[0] != '/')
        {
            ctx->pathbuf[0] = '/';
            grub_memcpy (ctx->pathbuf + 1, ctx->restrict_path, path_len);
            path_len++;
        }
        else
            grub_memcpy (ctx->pathbuf, ctx->restrict_path, path_len);

        if (ctx->pathbuf[path_len - 1] != '/')
        {
            ctx->pathbuf[path_len] = '/';
            ctx->pathbuf[path_len + 1] = '\0';
        }
        else
            ctx->pathbuf[path_len] = '\0';
    }
    else
    {
        ctx->pathbuf[0] = '/';
        ctx->pathbuf[1] = '\0';
    }

    if (ctx->progress && !ctx->quiet)
        grub_printf ("Scanning: %s (%s)...\n", name, ctx->fs->name);

    isoscan_scan_dir (ctx, ctx->pathbuf);

    grub_device_close (dev);
    if (grub_errno)
        grub_errno = GRUB_ERR_NONE;

    return 0;
}

/* ================================================================
   GRUB COMMAND REGISTRATION
   ================================================================ */
static grub_err_t
grub_cmd_isoscan (grub_extcmd_context_t ctxt,
                   int argc __attribute__ ((unused)),
                   char **args __attribute__ ((unused)))
{
    struct grub_arg_list *state = ctxt->state;
    struct isoscan_ctx ctx;
    char count_str[16];

    grub_memset (&ctx, 0, sizeof (ctx));
    ctx.root_only = state[ISOSCAN_ROOT_ONLY].set;
    ctx.restrict_path = state[ISOSCAN_PATH].set ? 
                        state[ISOSCAN_PATH].arg : NULL;
    ctx.quiet = state[ISOSCAN_QUIET].set;
    ctx.progress = state[ISOSCAN_PROGRESS].set;
    ctx.all_images = state[ISOSCAN_ALL_IMAGES].set;
    ctx.found = 0;
    ctx.visited = 0;

    if (!ctx.quiet)
        grub_printf (N_("isoscan: scanning all devices for bootable images...\n"));

    grub_device_iterate (isoscan_device_hook, &ctx);

    grub_snprintf (count_str, sizeof (count_str), "%d", ctx.found);
    grub_env_set ("isoscan_count", count_str);

    if (!ctx.quiet)
        grub_printf (N_("isoscan: found %d image(s).\n"), ctx.found);

    return GRUB_ERR_NONE;
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT (isoscan)
{
    cmd = grub_register_extcmd ("isoscan", grub_cmd_isoscan, 0,
                               N_("[-r] [-q] [-P] [-a] [-p PATH]"),
                               N_("Scan all disks for ISO/IMG/VHD files and add "
                                  "a bootable menu entry for each one found "
                                  "(Wboot automatic boot manager)."),
                               options);
}

GRUB_MOD_FINI (isoscan)
{
    grub_unregister_extcmd (cmd);
}