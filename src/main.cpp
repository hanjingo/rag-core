#include <iostream>
#if CRASH_HANDLER_ENABLE == 1
#include <hj/testing/crash.hpp>
#endif

#if TELEMETRY_ENABLE == 1
#include <hj/testing/telemetry.hpp>
#endif

#if LIC_ENABLE == 1
#include <hj/util/license.hpp>
#endif

// add your code here...
#include <hj/log/log.hpp>
#include <hj/os/env.h>
#include <hj/os/options.hpp>
#include <hj/os/signal.hpp>
#include <hj/io/file.hpp>

#include "err.h"
#include "global.h"
#include "util.h"
#include "server.h"
#include "conf.h"
#include "db_mgr.h"

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

    // init config
    if(!conf::instance().init())
    {
        throw std::runtime_error("Failed to load config file");
        return 0;
    }

    // log config
    auto filename =
        conf::instance().data().get<std::string>("log.filename", "default.log");
    auto max_size  = MB(conf::instance().data().get<int>("log.max_size", 1));
    auto max_files = conf::instance().data().get<int>("log.max_files", 1);
    auto min_lvl   = conf::instance().data().get<int>("log.min_lvl", 0);
    hj::log::logger::instance()->add_sink(
        hj::log::logger::instance()->create_rotate_file_sink(filename,
                                                             max_size,
                                                             max_files,
                                                             true));
    hj::log::logger::instance()->set_level(
        static_cast<hj::log::level>(min_lvl));
    LOG_INFO("init log with filename:{}, max_size:{}, max_files:{}, min_lvl:{}",
             filename,
             max_size,
             max_files,
             min_lvl);
    LOG_FLUSH();

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

    // load config file
    if(subcmd == "run")
    {
        // ./rag-core run
        // init dbs
        db_mgr::instance().init();

        // run server
        auto   addr = conf::instance().data().get<std::string>("server.addr");
        server srv(addr);
        LOG_DEBUG("starting server at {}", srv.address());
        srv.start();
        while(srv.is_running())
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        LOG_DEBUG("server stopped");
    } else if(subcmd == "prompt")
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
                      "run, prompt, set, get");
        });
        return 1;
    }
    return 0;
}