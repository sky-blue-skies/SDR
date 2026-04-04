load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
  name = "fm_demod",
  srcs = ["fm_demod.cc"],
  hdrs = ["include/fm_demod.h"],
  copts = [
    "-std=c++23",
    "-Isrc/dsp/include"
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "fir_filter",
  srcs = ["fir_filter.cc"],
  hdrs = ["include/fir_filter.h"],
  copts = [
    "-std=c++23",
    "-Isrc/dsp/include"
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "decimator",
  srcs = ["decimator.cc"],
  hdrs = ["include/decimator.h"],
  copts = [
    "-std=c++23",
    "-Isrc/dsp/include"
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "deemphasis",
  srcs = ["deemphasis.cc"],
  hdrs = ["include/deemphasis.h"],
  copts = [
    "-std=c++23",
    "-Isrc/dsp/include"
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "dc_blocker",
  srcs = ["dc_blocker.cc"],
  hdrs = ["include/dc_blocker.h"],
  copts = [
    "-std=c++23",
    "-Isrc/dsp/include"
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "agc",
  srcs = ["agc.cc"],
  hdrs = ["include/agc.h"],
  copts = [
    "-std=c++23",
    "-Isrc/dsp/include"
  ],
  visibility = ["//visibility:public"],
)