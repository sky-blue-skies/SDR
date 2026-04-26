load("@rules_cc//cc:defs.bzl", "cc_library")

genrule(
    name = "index_html_cc",
  srcs = ["www/index.html"],
  outs = ["include/index_html.h"],
    cmd = """
    mkdir -p include
    echo '#pragma once' > $@
    echo 'static const char kIndexHtml[] = R"HTML(' >> $@
    cat $< >> $@
    echo ')HTML";' >> $@
    """,
)

cc_library(
    name = "spectrum_server",
    srcs = ["spectrum_server.cc"],
    hdrs = [
      "include/spectrum_server.h",
      ":index_html_cc",
    ],
    includes = ["include"],
    copts = [
      "-std=c++23",
      "-Isrc/dsp/include"
    ],
    deps = ["//third_party/homebrew:libwebsockets"],
    visibility = ["//visibility:public"],
)