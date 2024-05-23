#include "media_player.h"
#include "docoder.h"
#include "DMADrawer.h"
#include <LittleFS.h>
#define FILENAME_MAX_LEN 100
enum FILE_TYPE : unsigned char
{
    FILE_TYPE_UNKNOW = 0,
    FILE_TYPE_FILE,
    FILE_TYPE_FOLDER
};
struct File_Info
{
    char *file_name;
    FILE_TYPE file_type;
    File_Info *front_node; // 上一个节点
    File_Info *next_node;  // 下一个节点
};
/*
 * get file basename
 */
static const char *get_file_basename(const char *path)
{
    // 获取最后一个'/'所在的下标
    const char *ret = path;
    for (const char *cur = path; *cur != 0; ++cur)
    {
        if (*cur == '/')
        {
            ret = cur + 1;
        }
    }
    return ret;
}

void join_path(char *dst_path, const char *pre_path, const char *rear_path)
{           
    while (*pre_path != 0)  
    {       
        *dst_path = *pre_path;
        ++dst_path;
        ++pre_path;
    }       
    if (*(pre_path - 1) != '/')
    {       
        *dst_path = '/';
        ++dst_path;
    }       
        
    if (*rear_path == '/')
    {   
        ++rear_path;
    }
    while (*rear_path != 0)
    {
        *dst_path = *rear_path;
        ++dst_path;
        ++rear_path; 
    }   
    *dst_path = 0;
}  
File_Info * listDir(const char *dirname)
{
    Serial.printf("Listing directory: %s\n", dirname);

    File root = LittleFS.open(dirname);
    if (!root)
    {
        Serial.println("Failed to open directory");
        return NULL;
    }
    if (!root.isDirectory())
    {
        Serial.println("Not a directory");
        return NULL;
    }

    int dir_len = strlen(dirname) + 1;

    // 头节点的创建（头节点用来记录此文件夹）
    File_Info *head_file = (File_Info *)malloc(sizeof(File_Info));
    head_file->file_type = FILE_TYPE_FOLDER;
    head_file->file_name = (char *)malloc(dir_len);
    // 将文件夹名赋值给头节点（当作这个节点的文件名）
    strncpy(head_file->file_name, dirname, dir_len - 1);
    head_file->file_name[dir_len - 1] = 0;
    head_file->front_node = NULL;
    head_file->next_node = NULL;

    File_Info *file_node = head_file;

    File file = root.openNextFile();
    while (file)
    {
        // if (levels)
        // {
        //     listDir(file.name(), levels - 1);
        // }
        const char *fn = get_file_basename(file.name());
        // 字符数组长度为实际字符串长度+1
        int filename_len = strlen(fn);
        if (filename_len > FILENAME_MAX_LEN - 10)
        {
            Serial.println("Filename is too long.");
        }

        // 创建新节点
        file_node->next_node = (File_Info *)malloc(sizeof(File_Info));
        // 让下一个节点指向当前节点
        // （此时第一个节点的front_next会指向head节点，等遍历结束再调一下）
        file_node->next_node->front_node = file_node;
        // file_node指针移向节点
        file_node = file_node->next_node;

        // 船家创建新节点的文件名
        file_node->file_name = (char *)malloc(filename_len);
        strncpy(file_node->file_name, fn, filename_len); //
        file_node->file_name[filename_len] = 0;          //
        // 下一个节点赋空
        file_node->next_node = NULL;

        char tmp_file_name[FILENAME_MAX_LEN] = {0};
        // sprintf(tmp_file_name, "%s/%s", dirname, file_node->file_name);
        join_path(tmp_file_name, dirname, file_node->file_name);
        if (file.isDirectory())
        {
            file_node->file_type = FILE_TYPE_FOLDER;
            // 类型为文件夹
            Serial.print("  DIR : ");
            Serial.println(tmp_file_name);
        }
        else
        {
            file_node->file_type = FILE_TYPE_FILE;
            // 类型为文件
            Serial.print("  FILE: ");
            Serial.print(tmp_file_name);
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }

        file = root.openNextFile();
    }

    if (NULL != head_file->next_node)
    {
        // 将最后一个节点的next_node指针指向 head_file->next_node
        file_node->next_node = head_file->next_node;
        // 调整第一个数据节点的front_node指针（非head节点）
        head_file->next_node->front_node = file_node;
    }
    return head_file;
}

#define MEDIA_PLAYER_APP_NAME "Media"

#define VIDEO_WIDTH 240L
#define VIDEO_HEIGHT 240L
#define MOVIE_PATH "/"
#define NO_TRIGGER_ENTER_FREQ_160M 90000UL // 无操作规定时间后进入设置160M主频（90s）
#define NO_TRIGGER_ENTER_FREQ_80M 120000UL // 无操作规定时间后进入设置160M主频（120s）

// 天气的持久化配置
#define MEDIA_CONFIG_PATH "/media.cfg"
struct MP_Config
{
    uint8_t switchFlag; // 是否自动播放下一个（0不切换 1自动切换）
    uint8_t powerFlag;  // 功耗控制（0低发热 1性能优先）
};

static void write_config(MP_Config *cfg){
}

static void read_config(MP_Config *cfg) {
}

struct MediaAppRunData
{
    PlayDocoderBase *player_docoder;
    unsigned long preTriggerKeyMillis; // 最近一回按键触发的时间戳
    int movie_pos_increate;
    File_Info *movie_file; // movie文件夹下的文件指针头
    File_Info *pfile;      // 指向当前播放的文件节点
    File file;
};

static MP_Config cfg_data;
static MediaAppRunData *run_data = NULL;

static File_Info *get_next_file(File_Info *p_cur_file, int direction)
{
    // 得到 p_cur_file 的下一个 类型为FILE_TYPE_FILE 的文件（即下一个非文件夹文件）
    if (NULL == p_cur_file)
    {
        return NULL;
    }

    File_Info *pfile = direction == 1 ? p_cur_file->next_node : p_cur_file->front_node;
    while (pfile != p_cur_file)
    {
        if (FILE_TYPE_FILE == pfile->file_type)
        {
            break;
        }
        pfile = direction == 1 ? pfile->next_node : pfile->front_node;
    }
    return pfile;
}

bool video_start(bool create_new)
{
    if (NULL == run_data->pfile)
    {
        // 视频文件夹空 就跳出去
        return false;
    }

    if (true == create_new)
    {
        run_data->pfile = get_next_file(run_data->pfile, run_data->movie_pos_increate);
    }

    char file_name[FILENAME_MAX_LEN] = {0};
    snprintf(file_name, FILENAME_MAX_LEN, "%s/%s", run_data->movie_file->file_name, run_data->pfile->file_name);

    run_data->file = LittleFS.open(file_name);
    if (NULL != strstr(run_data->pfile->file_name, ".mjpeg") || NULL != strstr(run_data->pfile->file_name, ".MJPEG"))
    {
        // 直接解码mjpeg格式的视频
        run_data->player_docoder = new MjpegPlayDocoder(&run_data->file, true);
        Serial.print(F("MJPEG video start --------> "));
    }
    else if (NULL != strstr(run_data->pfile->file_name, ".rgb") || NULL != strstr(run_data->pfile->file_name, ".RGB"))
    {
        // 使用RGB格式的视频
        run_data->player_docoder = new RgbPlayDocoder(&run_data->file, true);
        Serial.print(F("RGB565 video start --------> "));
    }

    Serial.println(file_name);
    return true;
}

static void release_player_docoder(void)
{
    // 释放具体的播放对象
    if (NULL != run_data->player_docoder)
    {
        delete run_data->player_docoder;
        run_data->player_docoder = NULL;
    }
}

int media_player_init()
{

    // 获取配置信息
    read_config(&cfg_data);
    // 初始化运行时参数
    // run_data = (MediaAppRunData *)malloc(sizeof(MediaAppRunData));
    // memset(run_data, 0, sizeof(MediaAppRunData));
    run_data = (MediaAppRunData *)calloc(1, sizeof(MediaAppRunData));
    run_data->player_docoder = NULL;
    run_data->movie_pos_increate = 1;
    run_data->movie_file = NULL; // movie文件夹下的文件指针头
    run_data->pfile = NULL;      // 指向当前播放的文件节点
    run_data->preTriggerKeyMillis = millis();

    run_data->movie_file = listDir(MOVIE_PATH);
    if (NULL != run_data->movie_file)
    {
        run_data->pfile = get_next_file(run_data->movie_file->next_node, 1);
    }

    // 设置CPU主频
    setCpuFrequencyMhz(240);

    // 创建播放
    video_start(false);
    return 0;
}

void media_player_process()
{
    if (!run_data->file)
    {
        Serial.println(F("Failed to open file for reading"));
        return;
    }

    if (run_data->file.available())
    {
        if(run_data->player_docoder != NULL)
        run_data->player_docoder->video_play_screen();
    }
    else
    {
        // 结束播放
        release_player_docoder();
        run_data->file.close();
        if (1 == cfg_data.switchFlag)
        {
            // 创建播放(重复播放)
            video_start(false);
        }
        else
        {
            // 创建播放(播放下一个)
            video_start(true);
        }
    }
}
void release_file_info(File_Info *info)
{
    File_Info *cur_node = NULL; // 记录当前节点
    if (NULL == info)
    {
        return;
    }
    for (cur_node = info->next_node; NULL != cur_node;)
    {
        // 判断是不是循环一圈回来了
        if (info->next_node == cur_node)
        {
            break;
        }
        File_Info *tmp = cur_node; // 保存准备删除的节点
        cur_node = cur_node->next_node;
        free(tmp);
    }
    free(info);
}

static int media_player_exit_callback(void *param)
{
    // 结束播放
    release_player_docoder();

    run_data->file.close(); // 退出时关闭文件
    // 释放文件循环队列
    release_file_info(run_data->movie_file);

    // 释放运行数据
    if (NULL != run_data)
    {
        free(run_data);
        run_data = NULL;
    }

    return 0;
}