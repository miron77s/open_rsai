#pragma once

#include <iostream>
#include <mutex>

static auto progress_dummy = [] ( float progress, const bool finished = false )
{
};

static auto progress_layers_dummy = [] ( int layer, int layers, float progress, const bool finished = false )
{
};

constexpr int console_progress_output_step  = 1;
constexpr int barWidth                      = 70;

static auto console_progress = [] ( float progress, const bool finished = false )
{
    std::cout << "[";
    int pos = barWidth * progress;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << int(progress * 1000.0f) / 10.0f << " % ";

    std::cout << ( ( !finished ) ? "\r" : "\n" );

    std::cout.flush();
};

static auto console_progress_layers = [] ( int layer, int layers, float progress, const bool finished = false )
{
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);

    static int call_count = 0;
    ++call_count;
    if ( call_count < console_progress_output_step && !finished )
        return;
    else
        call_count = 0;

    std::cout << "layer " << layer << "/" << layers << " ";
    console_progress ( progress, finished );
};
