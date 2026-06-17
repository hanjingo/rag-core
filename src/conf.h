#ifndef CONF_H
#define CONF_H

#include <vector>
#include <string>

#include <hj/encoding/ini.hpp>
#include <hj/util/license.hpp>

class conf
{
  public:
    conf();
    ~conf();

    static conf &instance()
    {
        static conf inst;
        return inst;
    }

    hj::ini data();

    std::string              issuer_id();
    hj::license::sign_algo   issuer_algo();
    std::vector<std::string> issuer_keys();
    int                      issuer_valid_times();
    int                      issuer_leeway();

    std::string              verifier_id();
    hj::license::sign_algo   verifier_algo();
    std::vector<std::string> verifier_keys();

  private:
    hj::ini _cfg;
};

#endif