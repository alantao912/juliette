//
// Created by Alan Tao on 10/28/2022.
//

#include "coach.h"

#include <fstream>
#include <cstring>
#include <iostream>

coach::coach(const char *info_dir) {
    std::ifstream info_file;
    info_file.open(info_dir);
    if (!info_file.is_open()) {
        std::cout << "Coach failed to start" << std::endl;
        return;
    }
    k_psqt = new int32_t[64];
    memset(k_psqt, 0, 64 * sizeof(int32_t));
    q_psqt = new int32_t[64];
    memset(q_psqt, 0, 64 * sizeof(int32_t));
    r_psqt = new int32_t[64];
    memset(r_psqt, 0, 64 * sizeof(int32_t));
    b_psqt = new int32_t[64];
    memset(b_psqt, 0, 64 * sizeof(int32_t));
    n_psqt = new int32_t[64];
    memset(n_psqt, 0, 64 * sizeof(int32_t));
    p_psqt = new int32_t[40];
    memset(p_psqt, 0, 40 * sizeof(int32_t));

    std::string line;
    while (getline(info_file, line)) {
        if (line.length() == 0) {
            continue;
        }
        std::string assignee = line.substr(0, line.find(' '));
        std::string value = line.substr(line.find('=') + 1);
        if (ends_with(assignee, "psqt_vec")) {
            int32_t *arr = nullptr, i = 0;
            int32_t start = value.find('{') + 1, end = start;

            switch (assignee.at(0)) {
                case 'k':
                    arr = this->k_psqt;
                    break;
                case 'q':
                    arr = this->q_psqt;
                    break;
                case 'r':
                    arr = this->r_psqt;
                    break;
                case 'n':
                    arr = this->n_psqt;
                    break;
                case 'b':
                    arr = this->b_psqt;
                    break;
                case 'p':
                    arr = this->p_psqt;
                    break;
                default:
                    std::cout << "Broke" << std::endl;
                    return;
            }
            while (value[end] != '}') {
                end = value.find(',', start);
                if (end != std::string::npos) {
                    goto START;
                }
                end = value.find('}', start);
                START:
                arr[i++] = atoi(value.substr(start, end).c_str());
                start = end + 1;
            }
        } else if (assignee == "WEIGHTS_DIR") {
            this->weights_dir = strdup(value.c_str());
        } else if (assignee == "WEIGHTS_SRC") {
            this->weights_src = strdup(value.c_str());
        } else if (assignee == "epoch") {
            this->epoch = atoi(value.c_str());
        } else if (assignee == "dx") {
            this->dx = atoi(value.c_str());
        }
    }
    info_file.close();
}

coach::~coach() {
    delete k_psqt;
    delete q_psqt;
    delete r_psqt;
    delete b_psqt;
    delete n_psqt;
    delete p_psqt;
}

bool coach::ends_with(const std::string &str, const char *suffix) {
    const size_t j = strlen(suffix), k = str.length();
    if (k < j) {
        return false;
    }
    for (size_t i = k - j; i < k; ++i) {
        if (str.at(i) != suffix[i - k + j]) {
            return false;
        }
    }
    return true;
}

void coach::save(const char *info_dir) {
    std::ofstream info_file;
    info_file.open(info_dir);
    info_file << "WEIGHTS_DIR =" << weights_dir << '\n';
    info_file << "WEIGHTS_SRC =" << weights_src << "\n\n";
    info_file << "epoch = " << epoch << '\n';
    info_file << "dx = " << dx << "\n\n";
    info_file << "k_psqt_vec = {";
    for (int i = 0; i < 64; ++i) {
        info_file << k_psqt[i];
        if (i == 63) {
            info_file << "}\n";
        } else {
            info_file << ',';
        }
    }
    info_file << "q_psqt_vec = {";
    for (int i = 0; i < 64; ++i) {
        info_file << q_psqt[i];
        if (i == 63) {
            info_file << "}\n";
        } else {
            info_file << ',';
        }
    }
    info_file << "r_psqt_vec = {";
    for (int i = 0; i < 64; ++i) {
        info_file << r_psqt[i];
        if (i == 63) {
            info_file << "}\n";
        } else {
            info_file << ',';
        }
    }
    info_file << "b_psqt_vec = {";
    for (int i = 0; i < 64; ++i) {
        info_file << b_psqt[i];
        if (i == 63) {
            info_file << "}\n";
        } else {
            info_file << ',';
        }
    }
    info_file << "n_psqt_vec = {";
    for (int i = 0; i < 64; ++i) {
        info_file << n_psqt[i];
        if (i == 63) {
            info_file << "}\n";
        } else {
            info_file << ',';
        }
    }
    info_file << "p_psqt_vec = {";
    for (int i = 0; i < 40; ++i) {
        info_file << p_psqt[i];
        if (i == 39) {
            info_file << "}\n";
        } else {
            info_file << ',';
        }
    }
}
