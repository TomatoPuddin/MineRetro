#pragma once

#include <cstdint>
#include "platform.h"

extern "C" {
#include "libretro.h"
}

namespace mineretro {
    struct LibretroReference {
        std::unique_ptr<Library> lib;
        bool initialized;
        bool supports_no_game;
        bool is_game_loaded;
        retro_perf_counter *perf_counter_last;

        decltype(&retro_init) retro_init;

        decltype(&retro_deinit) retro_deinit;

        decltype(&retro_api_version) retro_api_version;

        decltype(&retro_get_system_info) retro_get_system_info;

        decltype(&retro_get_system_av_info) retro_get_system_av_info;

        decltype(&retro_set_controller_port_device) retro_set_controller_port_device;

        decltype(&retro_reset) retro_reset;

        decltype(&retro_run) retro_run;

        decltype(&retro_load_game) retro_load_game;

        decltype(&retro_unload_game) retro_unload_game;
    };

    extern "C" {
    MR_API void MineretroLoadCore(const char *core_file);

    MR_API void MineretroUnloadCore();

    MR_API bool MineretroLoadGame(const char *game_file);

    MR_API void MineretroUnloadGame();

    MR_API void MineretroLoop();

    MR_API void MineretroSetVideo(retro_video_refresh_t video);

    MR_API void MineretroSetAudio(retro_audio_sample_t audio);

    MR_API void MineretroSetAudioBatch(retro_audio_sample_batch_t audio);

    MR_API void MineretroSetInputPoll(retro_input_poll_t input_poll);

    MR_API void MineretroSetInputState(retro_input_state_t input_state);

    MR_API void MineretroSetSystemAndSaveDir(char *system, char *save);

    MR_API retro_system_info MineretroGetSystemInfo();

    MR_API retro_system_av_info MineretroGetSystemAvInfo();

    MR_API retro_game_geometry MineretroGetGeometryInfo();

    MR_API retro_pixel_format MineretroGetPixelFormat();

    MR_API unsigned MineretroGetRotation();
    }

    bool RETRO_CALLCONV CoreEnvironment(unsigned cmd, void *data);

    void RETRO_CALLCONV CoreVideoRefresh(const void *data, unsigned width, unsigned height, size_t pitch);

    void RETRO_CALLCONV CoreInputPoll();

    int16_t RETRO_CALLCONV CoreInputState(unsigned port, unsigned device, unsigned index, unsigned id);

    size_t RETRO_CALLCONV CoreAudioSampleBatch(const int16_t *data, size_t frames);

    void RETRO_CALLCONV CoreAudioSample(int16_t left, int16_t right);

    void RETRO_CALLCONV CoreLog(retro_log_level level, const char *fmt, ...);
}
