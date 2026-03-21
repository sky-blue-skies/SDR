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