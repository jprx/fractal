#include <fractal.h>
#include <filesys/fat.h>

u32 get_cluster_for_directory(fat_dir_t *d) {
    if (!d) return 0;
    return ((d->first_cluster_low) | (d->first_cluster_hi << 16ull));
}

void init_fat32(fat32_bpb_t *initrd) {
    printf("initrd: 0x%X\r\n", initrd);
    printf("ebpb: 0x%X\r\n", &initrd->ebpb);
    if (EXPECTED_EBPB_OFFSET != (usize)&initrd->ebpb - (usize)initrd) {
        panic("something is wrong with the FAT32 headers");
    }
    if (initrd->ebpb.signature_word != FAT32_MAGIC) {
        panic("Invalid FAT32 signature word: 0x%X\r\n", initrd->ebpb.signature_word);
    }

    printf("Loading FAT32 filesystem ");
    for (usize i = 0; i < FAT32_VOLUME_NAMELEN; i++) {
        printf("%c", initrd->ebpb.volume_label[i]);
    }
    printf("\r\n");

    printf("bytes per sector: %d\r\n", initrd->bytes_per_sector);
    printf("sectors per cluster: %d\r\n", initrd->sectors_per_cluster);
    usize cluster_size = initrd->bytes_per_sector * initrd->sectors_per_cluster;
    printf("bytes per cluster: %d\r\n", cluster_size);
    printf("number of FATs: %d\r\n", initrd->num_fats);
    printf("fsinfo is at sector %d\r\n", initrd->ebpb.fsinfo_secnum);
    printf("reserved sectors: %d\r\n", initrd->reserved_sector_count);
    printf("root cluster: %d\r\n", initrd->ebpb.root_cluster);
    printf("total sectors: %d\r\n", initrd->total_sectors);

    if (initrd->num_fats != FAT32_EXPECTED_NUM_FATS) {
        panic("Incorrect number of FATs on this initrd: %d", initrd->num_fats);
    }

    // First FAT begins immediately after reserved sectors end
    // Reserved sectors are for the BPB and filesys info and other stuff
    fat_entry_t *fat_table = (fat_entry_t *)(((usize)initrd) + (initrd->bytes_per_sector * initrd->reserved_sector_count));
    printf("FAT table is at 0x%X\r\n", fat_table);
    usize fat_table_n_sectors = initrd->ebpb.table_size;
    usize fat_table_n_bytes = fat_table_n_sectors * initrd->bytes_per_sector;
    printf("FAT table has %d entries (0x%X bytes)\r\n", fat_table_n_sectors, fat_table_n_bytes);

    for (usize i = 0; i < fat_table_n_sectors; i++) {
        printf("Entry %d: 0x%X\r\n", i, fat_table[i]);
    }

    // Cluster 2 (the first cluster) is after two FAT tables
    usize data_cluster_begin = (((usize)fat_table)) + (((initrd->num_fats * fat_table_n_bytes)));
    printf("data cluster is at 0x%X\r\n", data_cluster_begin);

    fat_dir_t *rdir = (fat_dir_t *)data_cluster_begin;
    printf("Root dir name is %s, cluster %d\r\n", rdir[0].name, get_cluster_for_directory(&rdir[0]));
    printf("Next %s, cluster %d\r\n", rdir[1].name, get_cluster_for_directory(&rdir[1]));

    fat_dir_t *dir1 = (fat_dir_t *)(data_cluster_begin + 0x200);
    for (usize i = 0; i < 16; i++) {
        printf("dir1[%d] is %s (cluster %d)\r\n", i, dir1[i].name, get_cluster_for_directory(&dir1[i]));
    }

    panic("@TODO: Rest of FAT32 implementation");

    while(true);
}
