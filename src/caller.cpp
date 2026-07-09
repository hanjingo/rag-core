#include "caller.h"

#include <hj/log/logger.hpp>

int deepseek_caller::loop_query(
    const std::string                              &content,
    const std::string                              &api_key,
    const std::function<bool(std::string &output)> &callback)
{
    std::string      key = api_key.empty() ? _api_key : api_key;
    hj::http_request req;
    req.method  = "POST";
    req.path    = "/v1/chat/completions";
    req.headers = {{"Authorization", "Bearer " + key},
                   {"Content-Type", "application/json"},
                   {"Accept", "text/event-stream"}};

    std::string value = "User: " + content + "\nAI:";
    hj::json    params;
    params["model"]    = _model;
    params["messages"] = {{{"role", "user"}, {"content", value}}};
    params["stream"]   = true;
    req.body           = params.dump();

    // handle response
    req.response_handler = [this, callback](const hj::http_response &res) {
        if(res.status != 200)
            LOG_ERROR("HTTP Request failed with status: {}, body: {}",
                      res.status,
                      res.body);

        // return callback(output);
        return true;
    };

    // handle streaming data
    auto stream_buf = std::make_shared<std::string>();
    req.content_receiver =
        [this, callback, stream_buf](
            const char *data,
            size_t      data_length,
            size_t      offset,
            size_t      total_length) -> bool { // Handle streaming data
        stream_buf->append(data, data_length);
        // LOG_DEBUG("Received stream_buf:{}, data chunk: {} bytes, offset: {}, "
        //           "total_length: {}",
        //           *stream_buf,
        //           data_length,
        //           offset,
        //           total_length);

        // parse
        size_t pos = 0;
        while((pos = stream_buf->find("\n")) != std::string::npos)
        {
            std::string line = stream_buf->substr(0, pos);

            // \n
            stream_buf->erase(0, pos + 1);

            // \r\n
            if(!line.empty() && line.back() == '\r')
                line.pop_back();

            // empty line
            if(line.empty() || line[0] == ':')
                continue;

            // start with "data:""
            if(line.rfind("data: ", 0) != 0)
                continue;

            std::string json_str = line.substr(6);
            if(json_str == "[DONE]")
            {
                LOG_DEBUG("Stream finished.");
                return true;
            }

            try
            {
                // LOG_DEBUG("try to parse json_str:{}", json_str);
                auto json_data = hj::json::parse(json_str);
                if(json_data.contains("choices")
                   && !json_data["choices"].empty()
                   && json_data["choices"][0].contains("delta")
                   && json_data["choices"][0]["delta"].contains("content"))
                {
                    std::string content_piece =
                        json_data["choices"][0]["delta"]["content"];

                    if(!callback(content_piece))
                    {
                        return false;
                    }
                }
            }
            catch(const std::exception &e)
            {
                LOG_ERROR("JSON parse error: {}, Line content: {}",
                          e.what(),
                          json_str);
                return false;
            }
        }

        return true;
    };

    LOG_DEBUG("send before");
    // send request
    _cli.send(req);
    LOG_DEBUG("send after");

    // end
    return OK;
}

// ------------------------------ CALLER MGR ------------------------------
int caller_mgr::load(const std::string &id,
                     const std::string &type,
                     const int          timeout_sec,
                     const std::string &api_key)
{
    if(_callers.find(id) != _callers.end())
    {
        LOG_ERROR("Caller {} already loaded, skip", id);
        return LLM_ERR_MODEL_ALREADY_LOADED;
    }

    std::unique_ptr<caller> model;
    if(type == CALLER_TYPE_DEEPSEEK)
    {
        model = std::make_unique<deepseek_caller>(timeout_sec, api_key);
    } else
    {
        LOG_ERROR("Unsupported model type: {}", type);
        return LLM_ERR_MODEL_LOAD_FAIL;
    }

    if(!model)
    {
        LOG_ERROR("Failed to create caller for {}", id);
        return LLM_ERR_MODEL_LOAD_FAIL;
    }
    _callers[id] = std::move(model);
    LOG_INFO("Loaded caller {} with type {}", id, type);
    return OK;
}

int caller_mgr::loop_query(
    const std::string                              &id,
    const std::string                              &content,
    const std::string                              &api_key,
    const std::function<bool(std::string &output)> &callback)
{
    // Load model
    if(_callers.find(id) == _callers.end())
    {
        LOG_ERROR("Caller {} not found", id);
        return CALLER_ERR_NOT_EXIST;
    }

    auto caller = _callers.find(id)->second.get();
    return caller->loop_query(content, api_key, callback);
}