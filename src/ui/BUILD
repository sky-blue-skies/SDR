load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "spectrum_server",
    srcs = ["spectrum_server.cc"],
    hdrs = ["include/spectrum_server.h"],
    includes = ["include"],
    copts = [
      "-std=c++23",
      "-Isrc/dsp/include"
    ],
    deps = ["//third_party/homebrew:libwebsockets"],
    visibility = ["//visibility:public"],
)