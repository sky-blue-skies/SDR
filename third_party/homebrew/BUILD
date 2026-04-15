load("@rules_cc//cc:defs.bzl", "cc_library")
## Wrappers for homebrew installed libraries

cc_library(
  name = "rtlsdr",
  hdrs = glob(["rtlsdr_include/**"]),
  includes = ["rtlsdr_include"],
  linkopts = [
    "-L/opt/homebrew/lib",
    "-lrtlsdr",
  ],
  visibility = ["//visibility:public"],
) 

cc_library(
    name = "portaudio",
    hdrs = glob(["portaudio_include/**"]),
    includes = ["portaudio_include"],
    linkopts = [
        "-L/opt/homebrew/lib",
        "-lportaudio",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "fftw3f",
    hdrs = glob(["fftw_include/**"]),
    includes = ["fftw_include"],
    linkopts = [
        "-L/opt/homebrew/lib",
        "-lfftw3f",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "openssl",
    hdrs = glob(["openssl_include/**"]),
    includes = ["openssl_include"],
    linkopts = [
        "-L/opt/homebrew/opt/openssl/lib",
        "-lssl",
        "-lcrypto",
    ],
    visibility = ["//visibility:public"],
)


cc_library(
    name = "libwebsockets",
    hdrs = glob(["libwebsockets_include/**"]),
    includes = ["libwebsockets_include"],
    linkopts = [
        "-L/opt/homebrew/lib",
        "-lwebsockets",
    ],
    deps = [":openssl"],  
    visibility = ["//visibility:public"],
)
