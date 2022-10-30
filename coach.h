//
// Created by Alan Tao on 10/28/2022.
//

#ifndef JULIETTE_COACH_H
#define JULIETTE_COACH_H

#include <map>
#include <cstdint>

class coach {
private:

    std::map<std::string, std::string> properties;

    int32_t *k_psqt;
    int32_t *q_psqt;
    int32_t *r_psqt;
    int32_t *b_psqt;
    int32_t *n_psqt;
    int32_t *p_psqt;

    int32_t dx;
    int32_t epoch;

    const char *weights_dir;
    const char *weights_src;

    bool ends_with(const std::string &str, const char *suffix);
public:
    explicit coach(const char *info_dir);

    ~coach();

    void save(const char *info_dir);
};

#endif //JULIETTE_COACH_H
