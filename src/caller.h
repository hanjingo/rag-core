#ifndef CALLER_H
#define CALLER_H

#define OPENSSL_ENABLE 1
#include <hj/net/http/http_client.hpp>
#include <hj/net/http/http_request.hpp>
#include <hj/net/http/http_response.hpp>
#include <hj/encoding/json.hpp>

#include "err.h"
#include "global.h"

struct caller
{
    virtual int
    loop_query(const std::string                              &content,
               const std::string                              &api_key,
               const std::function<bool(std::string &output)> &callback) = 0;
};

class deepseek_caller : public caller
{
  public:
    explicit deepseek_caller(int timeout_sec, const std::string &api_key = "")
        : _cli{hj::http_ssl_client("api.deepseek.com")}
        , _model{"deepseek-chat"}
        , _api_key{api_key}
    {
        _cli.set_read_timeout(timeout_sec);
    }
    ~deepseek_caller() {}

    virtual int loop_query(
        const std::string                              &content,
        const std::string                              &api_key,
        const std::function<bool(std::string &output)> &callback) override;

  private:
    hj::http_ssl_client _cli;
    std::string         _model;
    std::string         _api_key; // optional, can be empty
};

// ------------------------------ CALLER MGR ------------------------------
class caller_mgr
{
  public:
    caller_mgr() {};
    ~caller_mgr() {};

    static caller_mgr &instance()
    {
        static caller_mgr inst;
        return inst;
    }

    int load(const std::string &id,
             const std::string &type,
             const int          timeout_sec,
             const std::string &api_key = "");

    int loop_query(const std::string                              &id,
                   const std::string                              &content,
                   const std::string                              &api_key,
                   const std::function<bool(std::string &output)> &callback);

  private:
    std::unordered_map<std::string, std::unique_ptr<caller>> _callers;
};

#endif // CALLER_H