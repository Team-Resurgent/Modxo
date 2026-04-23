Import("env")
from os.path import join
platform = env.PioPlatform()
FRAMEWORK_DIR = platform.get_package_dir("framework-arduinopico")
board = env.BoardConfig()
chip = board.get("build.mcu")

env.Append(CPPPATH=[
    join(FRAMEWORK_DIR, "pico-sdk", "src", "rp2_common", "pico_aon_timer", "include")
])
env.BuildSources(
    join("$BUILD_DIR", "PicoAON"),
    join(FRAMEWORK_DIR, "pico-sdk", "src", "rp2_common", "pico_aon_timer")
)

if chip == "rp2350":
    # pico-sdk/src/rp2_common/hardware_powman/include/hardware/powman.h
    env.Append(CPPPATH=[
        join(FRAMEWORK_DIR, "pico-sdk", "src", "rp2_common", "hardware_powman", "include")
    ])
    # pico-sdk/src/rp2_common/hardware_powman
    env.BuildSources(
        join("$BUILD_DIR", "Powman"),
        join(FRAMEWORK_DIR, "pico-sdk", "src", "rp2_common", "hardware_powman")
    )
