#include "watch_dog.h"

bool watch_dog::watch(const std::string &output)
{
    if(output.empty())
        return true;

    _repeats[output]++;
    return _max_repeats == -1 || _repeats[output] < _max_repeats;
}