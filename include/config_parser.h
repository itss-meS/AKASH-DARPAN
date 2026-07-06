#pragma once
// ============================================================
//  AKASH-DARPAN  —  config_parser.h
//  INI-style configuration file parser.
//  Reads/writes SystemConfig to akash_config.ini
// ============================================================
#include "akash_types.h"
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <cstdio>

class ConfigParser {
public:
    // ── Load config from .ini file ─────────────────────────
    static bool load(const std::string& path, SystemConfig& cfg) {
        std::ifstream f(path);
        if (!f.is_open()) {
            printf("[Config] File not found: %s  (using defaults)\n", path.c_str());
            return false;
        }

        std::unordered_map<std::string, std::string> kv;
        std::string line;
        while (std::getline(f, line)) {
            // Strip comments and whitespace
            auto hash = line.find('#');
            if (hash != std::string::npos) line = line.substr(0, hash);
            auto semi = line.find(';');
            if (semi != std::string::npos) line = line.substr(0, semi);
            trim(line);
            if (line.empty() || line[0] == '[') continue;

            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            trim(key); trim(val);
            kv[key] = val;
        }

        // ── Camera ────────────────────────────────────────
        if (kv.count("frame_width"))   cfg.frameWidth        = stoi(kv["frame_width"]);
        if (kv.count("frame_height"))  cfg.frameHeight       = stoi(kv["frame_height"]);
        if (kv.count("pixel_size_um")) cfg.pixelSizeUm       = stod(kv["pixel_size_um"]);

        // ── MLA ───────────────────────────────────────────
        if (kv.count("mla_grid_x"))    cfg.mlaGridX          = stoi(kv["mla_grid_x"]);
        if (kv.count("mla_grid_y"))    cfg.mlaGridY          = stoi(kv["mla_grid_y"]);
        if (kv.count("lenslet_um"))    cfg.lensletSizeUm     = stod(kv["lenslet_um"]);
        if (kv.count("focal_mm"))      cfg.focalLengthMm     = stod(kv["focal_mm"]);

        // ── Pupil ─────────────────────────────────────────
        if (kv.count("pupil_diam_mm")) cfg.pupilDiamMm       = stod(kv["pupil_diam_mm"]);

        // ── DM ────────────────────────────────────────────
        if (kv.count("dm_grid_x"))     cfg.dmGridX           = stoi(kv["dm_grid_x"]);
        if (kv.count("dm_grid_y"))     cfg.dmGridY           = stoi(kv["dm_grid_y"]);
        if (kv.count("dm_stroke_um"))  cfg.dmMaxStroke       = stod(kv["dm_stroke_um"]);
        if (kv.count("dm_coupling"))   cfg.dmCoupling        = stod(kv["dm_coupling"]);

        // ── AO ────────────────────────────────────────────
        if (kv.count("wavelength_nm")) cfg.wavelengthNm      = stod(kv["wavelength_nm"]);
        if (kv.count("cog_threshold")) cfg.centroidThreshold = stof(kv["cog_threshold"]);

        // ── Playback ──────────────────────────────────────
        if (kv.count("data_path"))     cfg.dataPath          = kv["data_path"];
        if (kv.count("playback_speed"))cfg.playbackSpeed     = stof(kv["playback_speed"]);

        printf("[Config] Loaded from %s\n", path.c_str());
        return true;
    }

    // ── Save config to .ini file ───────────────────────────
    static bool save(const std::string& path, const SystemConfig& cfg) {
        std::ofstream f(path);
        if (!f.is_open()) {
            fprintf(stderr, "[Config] Cannot write: %s\n", path.c_str());
            return false;
        }

        f << "# AKASH-DARPAN Configuration File\n"
          << "# Generated automatically — edit freely\n\n"

          << "[camera]\n"
          << "frame_width    = " << cfg.frameWidth        << "\n"
          << "frame_height   = " << cfg.frameHeight       << "\n"
          << "pixel_size_um  = " << cfg.pixelSizeUm       << "\n\n"

          << "[mla]\n"
          << "mla_grid_x     = " << cfg.mlaGridX          << "\n"
          << "mla_grid_y     = " << cfg.mlaGridY          << "\n"
          << "lenslet_um     = " << cfg.lensletSizeUm     << "\n"
          << "focal_mm       = " << cfg.focalLengthMm     << "\n\n"

          << "[pupil]\n"
          << "pupil_diam_mm  = " << cfg.pupilDiamMm       << "\n\n"

          << "[dm]\n"
          << "dm_grid_x      = " << cfg.dmGridX           << "\n"
          << "dm_grid_y      = " << cfg.dmGridY           << "\n"
          << "dm_stroke_um   = " << cfg.dmMaxStroke       << "\n"
          << "dm_coupling    = " << cfg.dmCoupling        << "\n\n"

          << "[ao]\n"
          << "wavelength_nm  = " << cfg.wavelengthNm      << "\n"
          << "cog_threshold  = " << cfg.centroidThreshold << "\n\n"

          << "[playback]\n"
          << "data_path      = " << cfg.dataPath          << "\n"
          << "playback_speed = " << cfg.playbackSpeed     << "\n";

        printf("[Config] Saved to %s\n", path.c_str());
        return true;
    }

    // ── Write default config ───────────────────────────────
    static void writeDefaults(const std::string& path) {
        SystemConfig cfg;
        save(path, cfg);
    }

private:
    static void trim(std::string& s) {
        const char* ws = " \t\r\n";
        s.erase(0, s.find_first_not_of(ws));
        auto last = s.find_last_not_of(ws);
        if (last != std::string::npos) s.erase(last + 1);
    }
};
