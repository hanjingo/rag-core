#ifndef ASR_H
#define ASR_H

#include <memory>

#include <hj/ai/asr.hpp>
#include <hj/sync/object_pool.hpp>

class asr_mgr
{
  public:
    asr_mgr();
    ~asr_mgr();

    static asr_mgr &instance()
    {
        static asr_mgr inst;
        return inst;
    }

    static hj::asr::ctx_params_t  create_ctx_params();
    static hj::asr::full_params_t create_full_params();

    int load(const std::string           &ctx_id,
             const std::string           &path,
             const hj::asr::ctx_params_t &config);

    int translate(std::string                  &segment,
                  const std::string            &ctx_id,
                  const std::vector<float>     &data,
                  const hj::asr::full_params_t &params);

  private:
    hj::object_pool<hj::asr::state_t *> _state_pool;
    std::unordered_map<std::string, std::unique_ptr<hj::asr::context>> _ctxs;
};

#endif // ASR_H