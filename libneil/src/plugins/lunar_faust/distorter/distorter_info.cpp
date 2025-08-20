#include "distorter_info.hpp"

dual_distorter_info::dual_distorter_info() {
    this->flags = zzub::plugin_flag_has_audio_output | zzub::plugin_flag_is_effect | zzub::plugin_flag_has_ports;
    this->min_tracks = 0;
    this->max_tracks = 0;
    this->name = "lunar_distorter";
    this->short_name = "lunar_distorter";
    this->author = "tnh";
    this->uri = "@libneil/effect/lunar_distorter";
}
