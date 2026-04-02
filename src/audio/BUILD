load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
  name = "audio_sink",
  srcs = ["audio_sink.cc"],
  hdrs = ["include/audio_sink.h"],
  copts = [
    "-std=c++23",
    "-Isrc/audio/include",
  ],
  deps = ["//third_party/homebrew:portaudio"],
  visibility = ["//visibility:public"],
)