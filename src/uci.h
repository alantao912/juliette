//
// Created by Alan Tao on 9/2/2022.
//

#pragma once

#include <cinttypes>
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <map>
#include <unordered_map>
#include <vector>

#include "search.h"
#include "tables.h"
#include "timeman.h"

#define BUFLEN 512
#define MAX_THREAD_COUNT 32

struct SearchContext;

enum option_t 
{
    contempt, debug, ownBook, threadCount, hashSize
};

struct info_t 
{
    int32_t score;

    move_t bestMove;
    std::chrono::milliseconds elapsedTime;

    void formatData(char *, size_t, bool) const;
};

void *threadFunction(void *);

struct UCI {

public:

    static const std::string idStr;

    static const std::string replies[8];

    UCI();

    ~UCI();

    void initializeUCI();

    void parseUCIString(const char *);

    void position(const std::vector<std::string> &);

    void go(const std::vector<std::string> &);

    void formatData();

    void reply();

    void setOption(const std::vector<std::string> &);

    const std::string &getOption(option_t) const;

    void setElapsedTime(const std::chrono::milliseconds &);

    void joinThreads();

private:

    std::unordered_map<option_t, std::string> options;

    char sendbuf[BUFLEN];

    bool boardInitialized;

    pthread_t threads[MAX_THREAD_COUNT];

    size_t nThreads;

    SearchContext *mainThread;

    SearchContext **helperThreads;

    void synchronizeSearchContexts();
};
