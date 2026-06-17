#ifndef AUTH_H
#define AUTH_H

#include <vector>
#include <memory>

#include <hj/util/license.hpp>
#include <hj/crypto/rsa.hpp>

#include "conf.h"

class issuer
{
  public:
    issuer()
        : _issuer{std::make_shared<hj::license::issuer>(
              conf::instance().issuer_id(),
              conf::instance().issuer_algo(),
              conf::instance().issuer_keys(),
              conf::instance().issuer_valid_times())}
    {
    }
    ~issuer() { _issuer.reset(); }

    static issuer &instance()
    {
        static issuer inst;
        return inst;
    }

    int issue(std::string                            &output,
              const std::string                      &licensee,
              const std::size_t                       leeway_days,
              const std::vector<hj::license::pair_t> &claims = {});

  private:
    std::shared_ptr<hj::license::issuer> _issuer;
};


// --------------------------- verifier ------------------------
class verifier
{
  public:
    verifier()
        : _verifier{std::make_shared<hj::license::verifier>(
              conf::instance().verifier_id(),
              conf::instance().verifier_algo(),
              conf::instance().verifier_keys())}
    {
    }

    ~verifier() { _verifier.reset(); }

    static verifier &instance()
    {
        static verifier inst;
        return inst;
    }

    int verify(const hj::license::token_t             &token,
               const std::string                      &licensee,
               const std::size_t                       leeway_days,
               const std::vector<hj::license::pair_t> &claims = {});

  private:
    std::shared_ptr<hj::license::verifier> _verifier;
};
#endif