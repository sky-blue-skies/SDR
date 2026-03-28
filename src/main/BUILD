load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "sdr",
    srcs = ["main.cc"],
    copts = [
        "-std=c++23",
        "-Isrc/dsp/include",
        "-Isrc/rtl/include"
    ],
    deps = [
        "//src/rtl:rtl_source",
        "//src/dsp:fm_demod",
        "//src/dsp:fir_filter",
        "//src/dsp:decimator"
    ],
)