load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "rtl_source",
    srcs = ["rtl_source.cc"],
    hdrs = ["include/rtl_source.h"],
    includes = ["include"],
    copts = ["-std=c++23"],
    deps = ["//third_party/homebrew:rtlsdr"],
    visibility = ["//visibility:public"],
)