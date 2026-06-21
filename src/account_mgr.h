#ifndef ACCOUNT_MGR_H
#define ACCOUNT_MGR_H

class account_mgr
{
  public:
    account_mgr()  = default;
    ~account_mgr() = default;

    static account_mgr &instance()
    {
        static account_mgr inst;
        return inst;
    }
};

#endif // ACCOUNT_MGR_H