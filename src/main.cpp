#include <iostream>
#include <hj/testing/crash.hpp>
#include <hj/testing/telemetry.hpp>
#include <hj/util/license.hpp>

// add your code here...
#include <hj/log/log.hpp>
#include <hj/os/env.h>
#include <hj/os/options.hpp>
#include <hj/os/signal.hpp>

#include "err.h"
#include "global.h"
#include "util.h"

int main(int argc, char *argv[])
{
#if CRASH_HANDLER_ENABLE == 1
// add crash handle support
#pragma message("crash handler enabled, initializing crash handler...")
    hj::crash_handler::instance()->prevent_set_unhandled_exception_filter();
    hj::crash_handler::instance()->set_local_path("./");
#endif

#if TELEMETRY_ENABLE == 1
// add telemetry support
#pragma message("telemetry enabled, initializing tracer...")
    auto tracer =
        hj::telemetry::make_otlp_file_tracer("otlp_call", "./telemetry.json");
#endif

#if LIC_ENABLE == 1
// add license check support
#pragma message("license check enabled, verifying license...")
    hj::license::verifier vef{LIC_ISSUER, hj::license::sign_algo::none, {}};
    auto                  err = vef.verify_file(LIC_FPATH, PACKAGE, 1);
    if(err)
    {
        std::cerr << "license verify failed with err: " << err.message()
                  << ", please check your license file: " << LIC_FPATH
                  << std::endl;
        return -1;
    }
#endif

    // add your code here...
    // catch signals
    hj::sighandler::instance().sigcatch({SIGABRT, SIGTERM}, [](int sig) {});

    // set log level
#ifdef DEBUG
    hj::log::logger::instance()->set_level(hj::log::level::debug);
#else
    hj::log::logger::instance()->set_level(hj::log::level::info);
#endif

    // add options parse support
    hj::options              opts;
    hj::error_handler<err_t> h{[](const char *src, const char *dst) {
        LOG_DEBUG("error handler state transition: {} -> {}", src, dst);
    }};
    if(argc < 2)
    {
        h.match(error(ERR_ARGC_TOO_LESS), [&](const err_t &e) {
            LOG_ERROR("Error: too few arguments", argc);
        });
        return 1;
    }

    // parse options
    opts.add<std::string>("subcmd", std::string(""), "subcommand");
    opts.add_positional("subcmd", 1);

    opts.add<std::string>("content", "", "input content");
    opts.add_positional("content", 2);

    std::string subcmd  = opts.parse<std::string>(argc, argv, "subcmd");
    auto        content = opts.parse<std::string>(argc, argv, "content");
    if(subcmd == "prompt")
    {
        // ./rag-core prompt "hello"
        LOG_DEBUG("prompt content:{}", content);
    } else if(subcmd == "set")
    {
        // ./rag-core set "model:qwen;slice_sz:100;"
        auto params = parse_set_param(content);
        for(size_t i = 0; i < params.size(); i += 2)
            LOG_DEBUG("set param key:{}, val:{}", params[i], params[i + 1]);
    } else if(subcmd == "get")
    {
        // ./rag-core get "model;slice_sz"
        auto params = parse_get_param(content);
        for(size_t i = 0; i < params.size(); i++)
            LOG_DEBUG("get param key:{}", params[i]);

    } else
    {
        h.match(error(ERR_INVALID_SUBCMD), [&](const err_t &e) {
            LOG_ERROR("Error: unknown subcommand: {}, we expected one of these "
                      "subcommands: [{}]",
                      subcmd,
                      "prompt, set, get");
        });
        return 1;
    }
    return 0;
}