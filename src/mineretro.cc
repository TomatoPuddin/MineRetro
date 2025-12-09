#include "mineretro.h"

#include <ctime>
#include <cstdio>
#include <cstdarg>
#include <format>
#include <iostream>
#include <array>

#include <unordered_map>

#include "platform.h"
#include "util/string_hasher.h"

namespace mineretro {
    static constexpr std::array<std::string_view, 4> kLogLevel{"Debug", "Info", "Warning", "Error"};
    static constexpr retro_log_level kMinLogLevel = RETRO_LOG_INFO;

    static LibretroReference libretro_reference{};

    struct GameOption {
        const std::string desc;
        std::string value;
    };

    static std::unordered_map<std::string, GameOption, GenericStringHasher, std::equal_to<>> g_vars;
    static retro_system_info system_info{};
    static retro_audio_callback audio_callback{};
    static retro_system_av_info av_info{};
    static retro_game_geometry geometry_info{};
    static retro_pixel_format pixel_format = RETRO_PIXEL_FORMAT_0RGB1555;

    static unsigned rotation = 0;
    static std::string system_dir;
    static std::string save_dir;

    static retro_video_refresh_t mineretro_video = nullptr;
    static retro_audio_sample_t mineretro_audio = nullptr;
    static retro_audio_sample_batch_t mineretro_audio_batch = nullptr;
    static retro_input_poll_t mineretro_input_poll = nullptr;
    static retro_input_state_t mineretro_input_state = nullptr;

    void MineretroLoadCore(const char *core_file) {
        auto res = Platform::get().LoadLib(core_file);
        if (!res) {
            CoreLog(RETRO_LOG_ERROR, "Failed to load core lib");
            return;
        }
        auto& lib = **res;

        decltype(&retro_set_environment) set_environment;
        decltype(&retro_set_video_refresh) set_video_refresh;
        decltype(&retro_set_input_poll) set_input_poll;
        decltype(&retro_set_input_state) set_input_state;
        decltype(&retro_set_audio_sample) set_audio_sample;
        decltype(&retro_set_audio_sample_batch) set_audio_sample_batch;

        auto result = [&]() -> Result<void> {
            MR_CHECK(lib.FindSymbol("retro_init", libretro_reference.retro_init), LoadRetroCore);
            MR_CHECK(lib.FindSymbol("retro_deinit", libretro_reference.retro_deinit), LoadRetroCore);
            MR_CHECK(lib.FindSymbol("retro_api_version", libretro_reference.retro_api_version), LoadRetroCore);
            MR_CHECK(lib.FindSymbol("retro_get_system_info", libretro_reference.retro_get_system_info), LoadRetroCore);
            MR_CHECK(lib.FindSymbol("retro_get_system_av_info", libretro_reference.retro_get_system_av_info), LoadRetroCore);
            MR_CHECK(lib.FindSymbol("retro_set_controller_port_device", libretro_reference.retro_set_controller_port_device), LoadRetroCore);
            MR_CHECK(lib.FindSymbol("retro_run", libretro_reference.retro_run), LoadRetroCore);
            MR_CHECK(lib.FindSymbol("retro_load_game", libretro_reference.retro_load_game), LoadRetroCore);
            MR_CHECK(lib.FindSymbol("retro_unload_game", libretro_reference.retro_unload_game), LoadRetroCore);

            MR_CHECK(lib.FindSymbol("retro_set_environment", set_environment), LoadRetroCore);
            MR_CHECK(lib.FindSymbol("retro_set_video_refresh", set_video_refresh), LoadRetroCore);
            MR_CHECK(lib.FindSymbol("retro_set_input_poll", set_input_poll), LoadRetroCore);
            MR_CHECK(lib.FindSymbol("retro_set_input_state", set_input_state), LoadRetroCore);
            MR_CHECK(lib.FindSymbol("retro_set_audio_sample", set_audio_sample), LoadRetroCore);
            MR_CHECK(lib.FindSymbol("retro_set_audio_sample_batch", set_audio_sample_batch), LoadRetroCore);

            return kSuccess;
        }();
        if (!result) {
            CoreLog(RETRO_LOG_ERROR, "Invalid core lib");
            return;
        }

        libretro_reference.lib = std::move(*res);
        CoreLog(RETRO_LOG_INFO, "API Version: %d", libretro_reference.retro_api_version());

        set_environment(CoreEnvironment);
        libretro_reference.retro_init();
        libretro_reference.initialized = true;

        set_video_refresh(CoreVideoRefresh);
        set_input_poll(CoreInputPoll);
        set_input_state(CoreInputState);
        set_audio_sample(CoreAudioSample);
        set_audio_sample_batch(CoreAudioSampleBatch);

        // 获取系统信息
        libretro_reference.retro_get_system_info(&system_info);
        CoreLog(RETRO_LOG_INFO, "Core loaded: %s", core_file);
    }

    void MineretroUnloadCore() {
        if (libretro_reference.initialized) {
            MineretroUnloadGame();
            libretro_reference.retro_deinit();
            libretro_reference.initialized = false;
            libretro_reference.lib.reset();
        }
        g_vars.clear();
    }

    bool MineretroLoadGame(const char *game_file) {
        retro_game_info info = {game_file, nullptr};
        rotation = 0;

        info.path = game_file;
        info.meta = "";
        info.data = nullptr;
        info.size = 0;

        // 如果文件名不为空
        if (game_file) {
            // 如果是不是 need_fullpath
            if (!system_info.need_fullpath) {
                // 打开文件（以二进制模式）
                FILE *file = fopen(game_file, "rb");
                // 获取文件大小
                fseek(file, 0, SEEK_END);
                info.size = ftell(file);
                // 将文件指针移回文件开头
                fseek(file, 0, SEEK_SET);
                // 为文件内容分配内存
                info.data = malloc(info.size);
                // 读取文件内容到缓冲区
                fread(const_cast<void *>(info.data), 1, info.size, file);
                // 关闭文件
                fclose(file);
            }
        }

        const bool result = libretro_reference.retro_load_game(&info);
        if (info.data) {
            free(const_cast<void *>(info.data));
        }
        if (result) {
            libretro_reference.is_game_loaded = true;
            CoreLog(RETRO_LOG_INFO, "Successfully loaded game: %s", game_file);
            // 获取视频信息
            libretro_reference.retro_get_system_av_info(&av_info);
            // 设置当前输入按键布局为 RETRO_DEVICE_JOYPAD
            libretro_reference.retro_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);
            // 启用音频
            if (audio_callback.set_state) {
                audio_callback.set_state(true);
            }
        } else {
            libretro_reference.is_game_loaded = false;
            CoreLog(RETRO_LOG_ERROR, "The core failed to load the content.");
        }
        return result;
    }

    void MineretroUnloadGame() {
        if (libretro_reference.is_game_loaded) {
            libretro_reference.retro_unload_game();
            CoreLog(RETRO_LOG_INFO, "Successfully unloaded game.");
        }
        libretro_reference.is_game_loaded = false;
    }

    void MineretroLoop() {
        if (audio_callback.callback) {
            audio_callback.callback();
        }
        libretro_reference.retro_run();
    }

    bool CoreEnvironment(unsigned cmd, void* data) {
        static_assert(std::is_same_v<retro_environment_t, decltype(&CoreEnvironment)>);

        // 这个选项每帧会执行一次，故放在最前面
        if (cmd == RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE) {
            *(bool *) data = false;
            return true;
        }

        switch (cmd) {
            case RETRO_ENVIRONMENT_SET_VARIABLES: {
                // 将 data 转换成 retro_variable[]，然后复制给 g_vars
                // 传入的数据类似于这样的格式：
                // { "foo_option", "Speed hack coprocessor X; false|true" }
                // { "sfb_max_players", "Maximum number of players allowed (Requires restart); 4|2|3|1|5|6|7|8" }
                // key 就是 foo_option, sfb_max_players
                // value 就是后面那部分
                // 后面那部分，分号之前的都是注释
                // 后面的是可用的值，用 | 分割
                const auto *vars = static_cast<const struct retro_variable *>(data);
                for (const retro_variable *v = vars; v->key; ++v) {
                    std::string_view desc(v->value);
                    auto result = g_vars.emplace(v->key, GameOption{std::string(desc), ""});

                    auto semicolon = desc.find(';');
                    auto first_pipe = desc.find('|');

                    semicolon++;
                    while (isspace(desc[semicolon])) {
                        semicolon++;
                    }
                    if (first_pipe != std::string::npos) {
                        result.first->second.value = desc.substr(semicolon, first_pipe - semicolon);
                    } else {
                        result.first->second.value = desc.substr(semicolon);
                    }
                }
                return true;
            }

            case RETRO_ENVIRONMENT_GET_VARIABLE: {
                // 把我们 RETRO_ENVIRONMENT_SET_VARIABLES 这一步得到的数据
                auto var = static_cast<retro_variable *>(data);
                auto result = g_vars.find(std::string_view(var->key));
                if (result != g_vars.end()) {
                    var->value = result->second.value.c_str();
                }
                return true;
            }

            case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: {
                auto *cb = (retro_log_callback *) data;
                cb->log = CoreLog;
                return true;
            }

            case RETRO_ENVIRONMENT_SET_ROTATION: {
                rotation = *static_cast<const unsigned *>(data);
                return true;
            }

            case RETRO_ENVIRONMENT_GET_PERF_INTERFACE: {
                return false;
            }

            case RETRO_ENVIRONMENT_GET_CAN_DUPE: {
                *(bool *) data = true;
                return true;
            }

            case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: {
                const auto fmt = static_cast<const retro_pixel_format *>(data);
                if (*fmt > RETRO_PIXEL_FORMAT_RGB565) {
                    return false;
                }
                pixel_format = *fmt;
                return true;
            }

            case RETRO_ENVIRONMENT_SET_HW_RENDER: {
                return false;
            }

            case RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK: {
                return false;
            }

            case RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK: {
                auto *audio_cb = static_cast<const struct retro_audio_callback *>(data);
                audio_callback = *audio_cb;
                return true;
            }

            case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY: {
                *static_cast<const char **>(data) = save_dir.c_str();
                return true;
            }
            case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: {
                *static_cast<const char **>(data) = system_dir.c_str();
                return true;
            }

            case RETRO_ENVIRONMENT_SET_GEOMETRY: {
                auto *info = static_cast<const struct retro_game_geometry *>(data);
                geometry_info = *info;
                return true;
            }

            case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME: {
                // 有些引擎自带游戏
                libretro_reference.supports_no_game = *(bool *) data;
                return true;
            }

            case RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE: {
                int *value = (int *) data;
                // 启用音频和视频
                // 这玩意儿写的太抽象了
                *value = 1 << 0 | 1 << 1;
                return true;
            }

            default: {
                CoreLog(RETRO_LOG_ERROR, "Unsupported env cmd: %d", cmd);
                return false;
            }
        }
    }

    void CoreVideoRefresh(const void *data, unsigned width, unsigned height, size_t pitch) {
        static_assert(std::is_same_v<retro_video_refresh_t, decltype(&CoreVideoRefresh)>);

        if (data == nullptr) {
            return;
        }
        if (mineretro_video != nullptr) {
            mineretro_video(data, width, height, pitch);
        }
    }

    void CoreInputPoll() {
        static_assert(std::is_same_v<retro_input_poll_t, decltype(&CoreInputPoll)>);
        if (mineretro_input_poll != nullptr) {
            mineretro_input_poll();
        }
    }

    int16_t CoreInputState(unsigned port, unsigned device, unsigned index, unsigned id) {
        static_assert(std::is_same_v<retro_input_state_t, decltype(&CoreInputState)>);
        if (mineretro_input_state != nullptr) {
            return mineretro_input_state(port, device, index, id);
        }
        return 0;
    }

    size_t CoreAudioSampleBatch(const int16_t *data, const size_t frames) {
        static_assert(std::is_same_v<retro_audio_sample_batch_t, decltype(&CoreAudioSampleBatch)>);
        if (mineretro_audio_batch != nullptr) {
            return mineretro_audio_batch(data, frames);
        }
        return 0;
    }

    void CoreAudioSample(const int16_t left, const int16_t right) {
        static_assert(std::is_same_v<retro_audio_sample_t, decltype(&CoreAudioSample)>);
        if (mineretro_audio != nullptr) {
            mineretro_audio(left, right);
        }
    }

    void MineretroSetVideo(const retro_video_refresh_t video) {
        mineretro_video = video;
    }

    void MineretroSetAudio(const retro_audio_sample_t audio) {
        mineretro_audio = audio;
    }

    void MineretroSetAudioBatch(const retro_audio_sample_batch_t audio) {
        mineretro_audio_batch = audio;
    }

    void MineretroSetInputPoll(const retro_input_poll_t input_poll) {
        mineretro_input_poll = input_poll;
    }

    void MineretroSetInputState(const retro_input_state_t input_state) {
        mineretro_input_state = input_state;
    }

    void MineretroSetSystemAndSaveDir(char *system, char *save) {
        system_dir = system;
        save_dir = save;
    }

    retro_system_info MineretroGetSystemInfo() {
        return system_info;
    }

    retro_system_av_info MineretroGetSystemAvInfo() {
        return av_info;
    }

    retro_game_geometry MineretroGetGeometryInfo() {
        return geometry_info;
    }

    retro_pixel_format MineretroGetPixelFormat() {
        return pixel_format;
    }

    unsigned MineretroGetRotation() {
        return rotation;
    }

    void CoreLog(const retro_log_level level, const char *fmt, ...) {
        // 有些核心会 log 刷屏
        if (level < kMinLogLevel) {
            return;
        }

        char msg_buffer[4096] = {};
        char time_buffer[16] = {};
        const time_t now = time(nullptr);
        va_list va;

        va_start(va, fmt);
        auto msg_size = vsnprintf(msg_buffer, sizeof(msg_buffer), fmt, va);
        va_end(va);
        std::string_view msg(msg_buffer, msg_size);

        const tm *tm_info = localtime(&now);
        auto time_size = strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", tm_info);
        std::string_view time(time_buffer, time_size);;

        // 判断最后一行是不是换行符
        if (msg.ends_with('\n')) {
            std::cerr << std::format("[{}] [{}] [Mineretro] {}", time, kLogLevel[level], msg);
        } else {
            std::cerr << std::format("[{}] [{}] [Mineretro] {}\n", time, kLogLevel[level], msg);
        }
    }
}
