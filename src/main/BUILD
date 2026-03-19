load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "sdr",
    srcs = ["main.cc"],
    copts = ["-std=c++23"],
    deps = [
        "//src/rtl:rtl_source",
    ],
)