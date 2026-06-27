#include "server_reactor.h"

#include <hj/log/log.hpp>
#include <hj/time/date_time.hpp>
#include <hj/algo/uuid.hpp>
#include <hj/db/sqlite.hpp>

#include "conf.h"
#include "db_mgr.h"
#include "auth.h"
#include "account_mgr.h"
#include "llm.h"
#include "router.h"
#include "watch_dog.h"
#include "sync.h"

QueryReactor::QueryReactor(grpc::CallbackServerContext   *ctx,
                           const ::GrpcLibrary::QueryReq *req)
    : _ctx(ctx)
    , _w_queue{conf::instance().sync_write_queue_size()}
{
    _session_id = req->id();
    _user_id    = req->user_id();
    _auth       = req->auth();
    _content    = req->content();
    _model      = req->model();

    thread_pool::instance().enqueue([this]() { this->_process(); });
}

QueryReactor::~QueryReactor()
{
    _push();
}

void QueryReactor::OnWriteDone(bool ok)
{
    if(!ok)
    {
        LOG_ERROR("Write failed or client disconnected for session_id: {}",
                  _session_id);
        _is_cancelled.store(true);
        Finish(grpc::Status(grpc::StatusCode::ABORTED, "Client disconnected"));
        return;
    }

    _push();
}

void QueryReactor::OnDone()
{
    delete this;
}

void QueryReactor::_process()
{
    int64_t msg_id = static_cast<int64_t>(hj::uuid::gen_u64());
    auto    now    = hj::date_time::now().ms_since_epoch();
    auto    sql    = hj::sqlite::mprintf(SQL_INSERT_MESSAGE,
                                         msg_id,
                                         _session_id,
                                         ROLE_USER,
                                         _content.c_str(),
                                         NONE_MSG_ID,
                                         now);
    if(db_mgr::instance().exec(DB_SQLITE, sql) != OK)
    {
        _send("", true, ERR_SQLITE_EXEC_FAIL);
        return;
    }

    auto tokens = llm_mgr::instance().tokenize(_model, _content, true, true);
    auto params = llm_mgr::instance().create_ctx_params(
        conf::instance().llm_ctx_window_sz());
    auto      max_repeates = conf::instance().llm_model_max_repeats(_model);
    watch_dog dog{max_repeates};
    auto ec = llm_mgr::instance().loop_query(_model,
                                             tokens,
                                             params,
                                             [&](std::string &output) -> bool {
                                                 // 1. if disconnect, stop
                                                 // 2. if write too much repeat words (bug), stop
                                                 // 3. ...
                                                 if(_is_cancelled.load())
                                                     return false;

                                                 if(output.empty())
                                                     return true;

                                                 if(!dog.watch(output))
                                                 {
                                                     _send(output, true, OK);
                                                     return false;
                                                 }

                                                 _answer += output;
                                                 _send(output, false, OK);
                                                 return true;
                                             });

    if(_is_cancelled.load())
        return;

    msg_id = static_cast<int64_t>(hj::uuid::gen_u64());
    now    = hj::date_time::now().ms_since_epoch();
    sql    = hj::sqlite::mprintf(SQL_INSERT_MESSAGE,
                                 msg_id,
                                 _session_id,
                                 ROLE_ASSISTANT,
                                 _answer.c_str(),
                                 NONE_MSG_ID,
                                 now);
    if(db_mgr::instance().exec(DB_SQLITE, sql) != OK)
    {
        _send("", true, ERR_SQLITE_EXEC_FAIL);
        return;
    }

    // send final words
    _send("", true, ec);
    Finish(grpc::Status::OK);
}

void QueryReactor::_send(const std::string &text,
                         bool               is_finished,
                         int                error_code)
{
    ::GrpcLibrary::QueryResp resp;
    resp.set_error_code(error_code);
    resp.set_id(_session_id);
    resp.set_content(text);
    resp.set_is_finished(is_finished);

    _w_queue.enqueue(resp);
    _push();
}

void QueryReactor::_push()
{
    if(_is_cancelled.load())
    {
        // Finish(grpc::Status::CANCELLED);
        return;
    }

    ::GrpcLibrary::QueryResp resp;
    while(_w_queue.try_dequeue(resp))
    {
        StartWrite(&resp);
    }
}