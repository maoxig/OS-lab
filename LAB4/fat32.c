#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include "fat32.h"
static inline int sector_to_offset(int sector);
static void parse_directory_entries(void *cluster_data, struct FilesInfo *files_info);
static struct FilesInfo *read_directory(int cluster_number);
static int find_in_directory(const char *filename, struct FilesInfo *dir_files);
static void *get_cluster_data(int cluster);
static int next_cluster(int cluster);
static int first_cluster(int cluster);
static int read_directory_entry(int dir_sector, int index, struct DirEntry *entry);
static int parse_path(const char *path, int *cluster, int *size);
struct Fat32BPB *hdr; // 指向 BPB 的数据
int mounted = -1;     // 是否已挂载成功

// 定义打开文件表
#define MAX_OPEN_FILES 128
struct OpenFile
{
    int cluster;
    int size;
};
struct OpenFile open_files[MAX_OPEN_FILES];

// 辅助函数：将扇区号转换为字节偏移量
static inline int sector_to_offset(int sector)
{
    return sector * hdr->BPB_BytsPerSec;
}
// 辅助函数：解析目录簇中的文件信息
static void parse_directory_entries(void *cluster_data, struct FilesInfo *files_info)
{
    struct DirEntry *entry = (struct DirEntry *)cluster_data;
    for (int i = 0; i < (hdr->BPB_BytsPerSec * hdr->BPB_SecPerClus) / sizeof(struct DirEntry); i++)
    {
        if (entry->DIR_Name[0] == 0x00)
        {
            // 到达目录的末尾
            break;
        }
        if (entry->DIR_Name[0] != 0xE5 && (entry->DIR_Attr & LONG_NAME_MASK) != LONG_NAME)
        {
            // 不是删除的条目，也不是长文件名条目
            printf(" entry_name: %s, entry_cluster:%d ,entry:%x\n", (char *)entry->DIR_Name, entry->DIR_FstClusHI << 16 | entry->DIR_FstClusLO, entry);
            strncpy((char *)files_info->files[files_info->size].DIR_Name, (char *)entry->DIR_Name, 11);
            files_info->files[files_info->size].DIR_FileSize = entry->DIR_FileSize;
            files_info->size++;
        }
        entry++;
    }
    printf("parse_directory_entries, files_info_size:%d", files_info->size);
}
// 辅助函数：读取目录内容
static struct FilesInfo *read_directory(int cluster_number)
{
    struct FilesInfo *files_info = malloc(sizeof(struct FilesInfo));
    if (!files_info)
        return NULL; // 内存分配失败
    files_info->files = malloc(MAX_OPEN_FILES * sizeof(struct FileInfo));
    if (!files_info->files)
    {
        free(files_info);
        return NULL; // 内存分配失败
    }
    files_info->size = 0;

    void *cluster_data;
    printf("try to read cluster data\n");
    while (cluster_number < 0x0FFFFFF8)
    { // 最后一个簇的标记
        cluster_data = get_cluster_data(cluster_number);
        printf("cluster_data_addr:%x\n", (void *)cluster_data);
        if (!cluster_data)
        {
            // 读取簇数据失败
            free(cluster_data);
            free(files_info->files);
            free(files_info);
            printf("cannot read cluster data\n");
            return NULL;
        }
        printf("succeed to read cluster data\n");
        parse_directory_entries(cluster_data, files_info);
        // printf("cluster_data_addr_new:%x\n",(void*)cluster_data);
        cluster_number = next_cluster(cluster_number);
    }

    return files_info;
}
// 辅助函数：在目录中查找文件或子目录
static int find_in_directory(const char *filename, struct FilesInfo *dir_files)
{
    for (int i = 0; i < dir_files->size; ++i)
    {
        struct FileInfo *file_info = &dir_files->files[i];

        // 获取文件名和扩展名
        char *file_name = strdup(filename);
        char *file_ext = strchr(file_name, '.');
        if (file_ext)
        {
            *file_ext = '\0'; // 替换'.'为null字符
            file_ext++;       // 指向扩展名
        }

        // 将文件名转换为大写
        char dos_filename[11];
        strncpy(dos_filename, file_name, 8); // 只复制前8个字符
        dos_filename[8] = '\0';              // 确保字符串以null终止
        for (int j = 0; dos_filename[j] != '\0'; ++j)
        {
            dos_filename[j] = toupper(dos_filename[j]); // 转换为大写
        }

        // 填充剩余的字符为空格
        for (int j = strlen(dos_filename); j < 8; ++j)
        {
            dos_filename[j] = ' '; // 填充剩余的字符为空格
        }

        // 检查文件名
        printf("Comparing '%s' with '%s'\n", dos_filename, file_info->DIR_Name);
        if (strncmp(dos_filename, file_info->DIR_Name, 8) == 0)
        {
            // 检查扩展名（如果有）TODO
            if (file_ext)
            {
                printf("has ext\n");
                char dos_extension[4];
                strncpy(dos_extension, file_ext, 3); // 复制扩展名
                dos_extension[3] = '\0';             // 确保字符串以null终止
                for (int j = 0; dos_extension[j] != '\0'; ++j)
                {
                    dos_extension[j] = toupper(dos_extension[j]); // 转换为大写
                }
                for (int j = strlen(dos_extension); j < 3; ++j)
                {
                    dos_extension[j] = ' '; // 填充剩余的字符为空格
                }
                if (strncmp(dos_extension, &file_info->DIR_Name[8], 3) != 0)
                {
                    continue; // 扩展名不匹配，继续查找
                }
            }
            else
            {
                // 如果没有扩展名，确保目录项的后三位是空格
                char *ext_check = &file_info->DIR_Name[8];
                if (ext_check[0] != ' ' || ext_check[1] != ' ' || ext_check[2] != ' ')
                {
                    continue; // 如果目录项的后缀不是空格，则继续查找
                }
            }
            printf("Finded_in_directory,i:%d\n", i);
            return i; // 返回文件在目录中的索引
        }
    }

    return -1; // 表示没有找到文件
}
// 辅助函数：获取指定簇的数据
static void *get_cluster_data(int cluster)
{

    int sector = hdr->BPB_RsvdSecCnt + (hdr->BPB_NumFATs * hdr->BPB_FATSz32) + ((cluster - 2) * hdr->BPB_SecPerClus);
    printf("get_cluster_data of cluster :%d, sector:%d\n", cluster, sector);
    return (void *)((char *)hdr + sector_to_offset(sector));
}

// 辅助函数：获取下一个簇号
static int next_cluster(int cluster)
{
    // 计算 FAT 表的起始扇区和偏移量
    int fat_sector = hdr->BPB_RsvdSecCnt + (cluster * 4 / (hdr->BPB_BytsPerSec));
    int fat_offset = (cluster * 4) % hdr->BPB_BytsPerSec;
    uint32_t *fat_entry = (uint32_t *)((char *)hdr + sector_to_offset(fat_sector) + fat_offset);
    printf("fat_entry:%x result:%d\n", fat_entry, *fat_entry & 0x0FFFFFFF);
    if (*fat_entry < 0xFFFFFF8)
    {
        printf("The last cluster!\n");
    } // 文件的最后一个簇
    return *fat_entry & 0x0FFFFFFF; // 清除最高位以获取下一个簇号
}

// 辅助函数：获取簇链中的第一个簇号
static int first_cluster(int cluster)
{
    // 计算 FAT 表的起始扇区和偏移量
    int fat_sector = hdr->BPB_RsvdSecCnt + (cluster / (hdr->BPB_BytsPerSec / 4));
    int fat_offset = (cluster % (hdr->BPB_BytsPerSec / 4)) * 4;
    uint32_t *fat_entry = (uint32_t *)((char *)hdr + sector_to_offset(fat_sector) + fat_offset);
    return *fat_entry; // 直接获取第一个簇号
}

// 辅助函数：读取目录项
static int read_directory_entry(int dir_cluster, int logical_index, struct DirEntry *entry)
{
    // 每个簇可以容纳的目录项数量
    int entries_per_cluster = (hdr->BPB_BytsPerSec * hdr->BPB_SecPerClus) / sizeof(struct DirEntry);
    // printf("entries_per_cluster:%d\n",entries_per_cluster);
    //  跟踪当前簇号和簇内的物理索引
    int current_cluster = dir_cluster;
    int physical_index = 0;

    // 遍历簇链直到到达所需的逻辑索引
    while (logical_index >= 0)
    {
        // 读取当前簇的数据
        void *dir_data = get_cluster_data(current_cluster);
        if (!dir_data)
        {
            // 无法获取簇数据，返回错误
            return -1;
        }

        // 遍历簇内的目录项
        for (int i = 0; i < entries_per_cluster; ++i)
        {
            struct DirEntry *target_entry = (struct DirEntry *)((char *)dir_data + i * sizeof(struct DirEntry));

            // 跳过删除的条目和长文件名条目
            if (target_entry->DIR_Name[0] == 0xE5 || (target_entry->DIR_Attr & LONG_NAME_MASK) == LONG_NAME)
            {
                continue;
            }
            printf("targe_entry:%x,i:%d\n", target_entry, i);
            // 如果到达所需的逻辑索引，复制目录项数据并返回
            if (physical_index == logical_index)
            {
                memcpy(entry, target_entry, sizeof(struct DirEntry));
                return 0; // 读取成功
            }

            // 否则，递增物理索引
            physical_index++;
        }

        // 获取下一个簇的编号
        int next_cluster_num = next_cluster(current_cluster);
        if (next_cluster_num < 0 || next_cluster_num >= 0x0FFFFFF8)
        {
            // 到达簇链的末尾或错误，返回错误
            return -1;
        }
        current_cluster = next_cluster_num;
    }

    // 如果逻辑索引超出范围，返回错误
    return -1;
}

// 路径解析函数
static int parse_path(const char *path, int *target_cluster, int *size)
{
    // 根目录的簇号是 2
    int current_cluster = hdr->BPB_RootClus; // 2

    // 复制路径以避免修改原始路径
    char *path_copy = strdup(path);
    if (!path_copy)
        return -1; // 内存分配失败

    struct FilesInfo *current_dir_files = read_directory(current_cluster);
    // printf("info addr:%x\n",(void*)current_dir_files);
    if (!current_dir_files)
    {
        free(path_copy);
        printf("current_dir_files is emptry\n");
        return -1;
    }
    printf("success find current_dir_files\n");
    char *token = strtok(path_copy, "/");
    while (token != NULL)
    {
        int file_index = find_in_directory(token, current_dir_files);
        if (file_index == -1)
        {
            // 如果在目录中找不到文件或子目录，释放当前目录资源并返回错误
            free(current_dir_files->files);
            free(current_dir_files);
            free(path_copy);
            return -1;
        }

        // 获取当前文件或目录的信息
        struct DirEntry *dir_entry = malloc(sizeof(struct DirEntry));
        read_directory_entry(current_cluster, file_index, dir_entry);

        // 如果找到的是文件，则返回
        if ((dir_entry->DIR_Attr & DIRECTORY) == 0)
        { // 不是目录
            printf("is not a directory!\n");
            *target_cluster = (dir_entry->DIR_FstClusHI << 16) | dir_entry->DIR_FstClusLO; // TODO error cluster
            printf("??cluster:%d\n", *target_cluster);
            *size = current_dir_files->files[file_index].DIR_FileSize;
            free(current_dir_files->files);
            free(current_dir_files);
            return 0; // 返回 0 表示解析成功
        }

        // 更新当前簇号
        current_cluster = (dir_entry->DIR_FstClusHI << 16) | dir_entry->DIR_FstClusLO;

        // 读取下一个目录
        free(current_dir_files->files);
        free(current_dir_files);
        current_dir_files = read_directory(current_cluster);
        if (!current_dir_files)
        {
            free(path_copy);
            return -1;
        }

        token = strtok(NULL, "/");
    }

    free(current_dir_files->files);
    free(current_dir_files);
    free(path_copy);

    *target_cluster = current_cluster;
    return 0;
}

/////////
// 挂载磁盘镜像
int fat_mount(const char *path)
{
    if (mounted == 0)
    { // 重复挂载
        return -1;
    }
    // 只读模式打开磁盘镜像
    int fd = open(path, O_RDWR);
    if (fd < 0)
    {
        // 打开失败
        return -1;
    }
    // 获取磁盘镜像的大小
    off_t size = lseek(fd, 0, SEEK_END);
    if (size == -1)
    {
        // 获取失败
        return -1;
    }
    // 将磁盘镜像映射到内存
    hdr = (struct Fat32BPB *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (hdr == (void *)-1)
    {
        // 映射失败
        return -1;
    }
    // 关闭文件
    close(fd);

    assert(hdr->Signature_word == 0xaa55);                   // MBR 的最后两个字节应该是 0x55 和 0xaa
    assert(hdr->BPB_TotSec32 * hdr->BPB_BytsPerSec == size); // 不相等表明文件的 FAT32 文件系统参数可能不正确
    // Data Region Start = BPB_RsvdSecCnt + (BPB_NumFATs * BPB_FATSz32) + (BPB_RootEntCnt * sizeof(struct DirEntry))
    // date_region_start = hdr->BPB_RsvdSecCnt + (hdr->BPB_NumFATs *hdr->BPB_FATSz32)+ (hdr->BPB_rootEntCnt*sizeof(struct  DirEntry));

    // printf("hdr addr:%x\n",(uint32_t)(void*)hdr);
    //  打印 BPB 的部分信息
    printf("Some of the information about BPB is as follows\n");
    printf("OEM-ID \"%s\", \n", hdr->BS_oemName);
    printf("sectors/cluster %d, \n", hdr->BPB_SecPerClus);
    printf("bytes/sector %d, \n", hdr->BPB_BytsPerSec);
    printf("sectors %d, \n", hdr->BPB_TotSec32);
    printf("sectors/FAT %d, \n", hdr->BPB_FATSz32);
    printf("FATs %d, \n", hdr->BPB_NumFATs);
    printf("reserved sectors %d, \n", hdr->BPB_RsvdSecCnt);

    // 挂载成功
    mounted = 0;
    return 0;
}

// 打开文件
int fat_open(const char *path)
{
    if (mounted != 0)
        return -1; // 未挂载

    // 解析路径，找到文件对应的起始簇号和大小
    int cluster, size;
    int result = parse_path(path, &cluster, &size);

    struct DirEntry *dir_entry = (struct DirEntry *)get_cluster_data(cluster);
    if ((dir_entry->DIR_Attr & DIRECTORY) != 0) // 是一个目录
    {
        return -1;
    }

    if (result != 0)
    {
        printf("path_parse failed!\n");
        return -1; // 路径解析失败
    }

    for (int i = 0; i < MAX_OPEN_FILES; ++i)
    {
        if (open_files[i].cluster == 0)
        {                                    // 找到一个空闲的文件描述符
            open_files[i].cluster = cluster; // 假定 cluster 是路径解析后得到的簇号
            open_files[i].size = size;       // 假定 size 是路径解析后得到的文件大小
            printf("cluster:%d,size:%d,fd:%d\n", cluster, size, i);
            return i; // 返回文件描述符
        }
    }
    printf("cannot open more file!\n");
    return -1; // 打开文件数达到上限
}

// 关闭文件
int fat_close(int fd)
{
    if (fd < 0 || fd >= MAX_OPEN_FILES || open_files[fd].cluster == 0)
        return -1;

    open_files[fd].cluster = 0; // 清空文件描述符
    open_files[fd].size = 0;
    return 0;
}

// 读取普通文件内容
int fat_pread(int fd, void *buffer, int count, int offset)
{
    // 在读取到文件末尾时，返回值可能会小于 count；
    // 如果输入参数 count 值为 0 或者 offset 超过了文件末尾，则应返回 0。
    // 对于其它读取失败的情况，返回 -1。
    if (fd < 0 || fd >= MAX_OPEN_FILES || open_files[fd].cluster == 0)
        return -1;
    // 获取文件大小
    int fileSize = open_files[fd].size;

    // 当count为0或者offset超过文件末尾时返回0
    if (count == 0 || offset >= fileSize)
        return 0;

    // 计算实际可读取的最大字节数
    if (offset + count > fileSize)
        count = fileSize - offset;

    // 计算读取的起始簇和偏移量
    int cluster = open_files[fd].cluster;
    int cluster_size = hdr->BPB_BytsPerSec * hdr->BPB_SecPerClus;
    int cluster_offset = offset % cluster_size;
    int start_cluster = offset / cluster_size;

    // 跳转到正确的簇
    for (int i = 0; i < start_cluster; ++i)
    {
        cluster = next_cluster(cluster);
    }

    // 读取数据
    int bytes_read = 0;
    char *buf = (char *)buffer;
    while (bytes_read < count)
    {
        // 计算当前簇可读字节数
        int readable = cluster_size - cluster_offset;
        if (readable > count - bytes_read)
            readable = count - bytes_read;

        // 读取数据到缓冲区
        void *data = get_cluster_data(cluster);
        memcpy(buf + bytes_read, data + cluster_offset, readable);
        bytes_read += readable;
        cluster_offset = 0; // 下一个簇从开始位置读

        // 检查是否需要读取下一个簇
        if (bytes_read < count)
        {
            cluster = next_cluster(cluster);
            if (cluster == 0)
                break; // 文件结束
        }
    }
    return bytes_read;
}

// 读取目录内容的函数
struct FilesInfo *fat_readdir(const char *path)
{
    int cluster;
    int size;
    if (parse_path(path, &cluster, &size) != 0)
    {
        return NULL;
    }
    return read_directory(cluster);
}