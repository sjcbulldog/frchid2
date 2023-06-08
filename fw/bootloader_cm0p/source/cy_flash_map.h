/* AUTO-GENERATED FILE, DO NOT EDIT. ALL CHANGES WILL BE LOST! */
/* Platform: PSOC_062_2M */

static struct flash_area flash_areas[] = {
    {
        .fa_id        = FLASH_AREA_BOOTLOADER,
        .fa_device_id = FLASH_DEVICE_INTERNAL_FLASH,
        .fa_off       = 0x0U,
        .fa_size      = 0x18000U
    },
    {
        .fa_id        = FLASH_AREA_IMG_1_PRIMARY,
        .fa_device_id = FLASH_DEVICE_INTERNAL_FLASH,
        .fa_off       = 0x18000U,
        .fa_size      = 0xee000U
    },
    {
        .fa_id        = FLASH_AREA_IMG_1_SECONDARY,
        .fa_device_id = FLASH_DEVICE_INTERNAL_FLASH,
        .fa_off       = 0x106000U,
        .fa_size      = 0xee000U
    },
    {
        .fa_id        = FLASH_AREA_IMAGE_SWAP_STATUS,
        .fa_device_id = FLASH_DEVICE_INTERNAL_FLASH,
        .fa_off       = 0x1f8000U,
        .fa_size      = 0x4000U
    },
    {
        .fa_id        = FLASH_AREA_IMAGE_SCRATCH,
        .fa_device_id = FLASH_DEVICE_INTERNAL_FLASH,
        .fa_off       = 0x1f4000U,
        .fa_size      = 0x4000U
    }
};

struct flash_area *boot_area_descs[] = {
    &flash_areas[0U],
    &flash_areas[1U],
    &flash_areas[2U],
    &flash_areas[3U],
    &flash_areas[4U],
    NULL
};
