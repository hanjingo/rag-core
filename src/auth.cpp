#include "auth.h"
#include "err.h"

#include <iostream>

int issuer::issue(hj::license::token_t                   &output,
                  const std::string                      &licensee,
                  const std::size_t                       expired_days,
                  const std::vector<hj::license::pair_t> &claims)
{
    hj::license::err_t err =
        _issuer->issue(output, licensee, expired_days, claims);
    return err ? err.value() : OK;
}

int verifier::verify(const hj::license::token_t             &token,
                     const std::string                      &licensee,
                     const std::vector<hj::license::pair_t> &claims)
{
    hj::license::err_t err = _verifier->verify(token, licensee, claims);
    return err ? err.value() : OK;
}