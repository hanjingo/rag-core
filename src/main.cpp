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
#include "llm.h"
#include "caller.h"
#include "asr.h"

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

    // log config
    auto filename  = conf::instance().log_filename();
    auto max_size  = MB(conf::instance().log_max_size());
    auto max_files = conf::instance().log_max_files();
    auto min_lvl   = conf::instance().log_min_lvl();
    auto flush_on  = conf::instance().log_flush_on();
    hj::log::logger::instance()->add_sink(
        hj::log::logger::instance()->create_rotate_file_sink(filename,
                                                             max_size,
                                                             max_files,
                                                             true));
    hj::log::logger::instance()->set_level(
        static_cast<hj::log::level>(min_lvl));
    hj::log::logger::instance()->flush_on(
        static_cast<hj::log::level>(flush_on));
    LOG_INFO("init log with filename:{}, max_size:{}, max_files:{}, "
             "min_lvl:{}, flush_on:{}",
             filename,
             max_size,
             max_files,
             min_lvl,
             flush_on);

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

        // init llm remote apis
        auto apis = conf::instance().llm_remote_apis();
        LOG_DEBUG("init remote api config num: {}", apis.size());
        for(const auto &api : apis)
        {
            caller_mgr::instance().load(api.second.id,
                                        api.second.type,
                                        api.second.timeout_sec,
                                        api.second.api_key);
            LOG_INFO(
                "init remote api id:{}, type:{}, timeout_sec:{}, api_key:{}",
                api.second.id,
                api.second.type,
                api.second.timeout_sec,
                api.second.api_key);
        }

        // init models
        auto models = conf::instance().llm_models();
        LOG_DEBUG("init model config num: {}", models.size());
        for(const auto &model : models)
        {
            auto param            = llm_mgr::instance().create_model_params();
            param.n_gpu_layers    = model.second.n_gpu_layers;
            param.split_mode      = model.second.split_mode;
            param.main_gpu        = model.second.main_gpu;
            param.vocab_only      = model.second.vocab_only;
            param.use_mmap        = model.second.use_mmap;
            param.use_direct_io   = model.second.use_direct_io;
            param.use_mlock       = model.second.use_mlock;
            param.check_tensors   = model.second.check_tensors;
            param.use_extra_bufts = model.second.use_extra_bufts;
            param.no_host         = model.second.no_host;
            param.no_alloc        = model.second.no_alloc;
            llm_mgr::instance().load(model.second.id, model.second.path, param);
            LOG_INFO("init model id:{}, path:{}, n_gpu_layers:{}, "
                     "split_mode:{}, main_gpu:{}, vocab_only:{}, use_mmap:{}, "
                     "use_direct_io:{}, use_mlock:{}, check_tensors:{}, "
                     "use_extra_bufts:{}, no_host:{}, no_alloc:{}",
                     model.second.id,
                     model.second.path,
                     param.n_gpu_layers,
                     static_cast<int>(param.split_mode),
                     param.main_gpu,
                     param.vocab_only,
                     param.use_mmap,
                     param.use_direct_io,
                     param.use_mlock,
                     param.check_tensors,
                     param.use_extra_bufts,
                     param.no_host,
                     param.no_alloc);
        }
        LOG_INFO("init llm model finish");

        // init asr
        auto asr_ctxs = conf::instance().asr_ctxs();
        LOG_DEBUG("init asr ctx config num: {}", asr_ctxs.size());
        for(const auto &ctx : asr_ctxs)
        {
            auto param       = asr_mgr::instance().create_ctx_params();
            param.use_gpu    = ctx.second.use_gpu;
            param.gpu_device = ctx.second.gpu_device;
            asr_mgr::instance().load(ctx.second.id, ctx.second.path, param);
            LOG_INFO("init asr ctx id:{}, path:{}, use_gpu:{}, gpu_device:{}",
                     ctx.second.id,
                     ctx.second.path,
                     param.use_gpu,
                     param.gpu_device);
        }
        LOG_INFO("init asr ctx finish");

        // run server
        auto   addr = conf::instance().server_addr();
        server srv(addr);
        LOG_INFO("init core service finish");
        LOG_INFO("starting server at {}", srv.address());
        srv.start();
        while(srv.is_running())
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        LOG_INFO("server stopped");
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