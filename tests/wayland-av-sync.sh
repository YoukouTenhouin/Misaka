#!/bin/sh
# Run the CTest suite on KDE Wayland with a dedicated audio monitor.
set -eu
sink=misaka_playback_test_$$
module=$(pactl load-module module-null-sink sink_name="$sink")
trap 'pactl unload-module "$module"' EXIT
PULSE_SINK="$sink" MISAKA_TEST_MONITOR="$sink.monitor" QT_QPA_PLATFORM=wayland \
    ctest --test-dir "${1:-build}" --verbose
