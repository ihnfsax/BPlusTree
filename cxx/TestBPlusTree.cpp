#include "TestBPlusTree.hpp"
#include "BPlusTree.hpp"
#include "Serialization.hpp"
#include <algorithm>
#include <map>
#include <random>
#include <sys/time.h>

#define REPEAT 200
#define STRING_SIZE 1
#define DEFAULT_ORDER 64
#define BEST_ORDER 8

void TestBPlusTree::testAll(const int& stage) {
    if (stage == 1 || stage <= 0 || stage > 5) {
        fprintf(fp, "Stage 1 : measure time complexity (%d keys per node) \n", DEFAULT_ORDER);
        measureTimeComplexity();
    }
    if (stage == 2 || stage <= 0 || stage > 5) {
        int scale = 1000000;
        fprintf(fp, "Stage 2 : measure effect of order (%d entries) \n", scale);
        measureEffectOfOrder(scale);
    }
    if (stage == 3 || stage <= 0 || stage > 5) {
        fprintf(fp, "Stage 3 : measure serialization and deserialization cost (%d keys per node)\n", DEFAULT_ORDER);
        measureSerialize();
    }
    if (stage == 4 || stage <= 0 || stage > 5) {
        fprintf(fp, "Stage 4 : measure performance (%d keys per node)\n", BEST_ORDER);
        measurePerformance();
    }
}

void TestBPlusTree::measureTimeComplexity() {
    fflush(fp);
    std::uniform_int_distribution<int> dist;
    std::random_device                 rd;
    std::default_random_engine         rng{ rd() };
    my::BPlusTree<int, std::string>    btree(DEFAULT_ORDER);
    double                             insertTime, eraseTime;
    for (size_t tidx = 0; tidx <= tmidx;) {
        if (btree.size() == treeSize[tidx]) {
            insertTime = testInsert(btree);
            eraseTime  = testInsert(btree);
            fprintf(fp, "      ├── size: %8d  ncount: %7d  height: %2d  insert: %9.6lfus  erase: %9.6lfus\n",
                    btree.size(), btree.ncount(), btree.height(), insertTime, eraseTime);
            fflush(fp);
            tidx++;
        }
        btree.insert(dist(rng), std::string(STRING_SIZE, 'a'));
    }
    fprintf(fp, "      └── finished\n");
    fflush(fp);
}

void TestBPlusTree::measureEffectOfOrder(const int& scale) {
    fflush(fp);
    std::uniform_int_distribution<int> dist;
    std::random_device                 rd;
    std::default_random_engine         rng{ rd() };
    double                             insertTime, eraseTime;
    for (size_t oidx = 0; oidx <= omidx; ++oidx) {
        my::BPlusTree<int, std::string> btree(orderSize[oidx]);
        while (btree.size() < scale) {
            btree.insert(dist(rng), std::string(STRING_SIZE, 'a'));
        }
        insertTime = testInsert(btree);
        eraseTime  = testInsert(btree);
        fprintf(fp, "      ├── order: %4d  ncount: %6d  height: %2d  insert: %10.6lfus  erase: %10.6lfus\n",
                btree.order(), btree.ncount(), btree.height(), insertTime, eraseTime);
        fflush(fp);
    }
    fprintf(fp, "      └── finished\n");
    fflush(fp);
}

void TestBPlusTree::measureSerialize() {
    fflush(fp);
    std::uniform_int_distribution<int> dist;
    std::random_device                 rd;
    std::default_random_engine         rng{ rd() };
    my::BPlusTree<int, std::string>    btree(DEFAULT_ORDER);
    double                             serializeTime, deserializeTime;
    off_t                              fileSize;
    struct timespec                    t1, t2;
    char                               filePath[] = "./testXXXXXX";
    int                                fd;
    if ((fd = mkstemp(filePath)) < 0)
        throw std::runtime_error("mkstemp error");
    close(fd);
    my::Serialization serialization;
    for (size_t tidx = 0; tidx <= tmidx;) {
        if (btree.size() == treeSize[tidx]) {
            clock_gettime(CLOCK_REALTIME, &t1);
            fileSize = serialization.serialize(btree, filePath);
            clock_gettime(CLOCK_REALTIME, &t2);
            serializeTime = getTimeDifference(t2, t1);
            clock_gettime(CLOCK_REALTIME, &t1);
            my::BPlusTree<int, std::string>* btreePtr = serialization.deserialize<int, std::string>(filePath);
            clock_gettime(CLOCK_REALTIME, &t2);
            deserializeTime = getTimeDifference(t2, t1);
            delete btreePtr;
            fprintf(fp,
                    "      ├── size: %8d  ncount: %6d  height: %2d  serialize: %10.4lfms  deserialize: %10.4lfms  "
                    "file size: %9ldB\n",
                    btree.size(), btree.ncount(), btree.height(), serializeTime / 1000, deserializeTime / 1000,
                    fileSize);
            fflush(fp);
            tidx++;
        }
        btree.insert(dist(rng), std::string(STRING_SIZE, 'a'));
    }
    unlink(filePath);
    fprintf(fp, "      └── finished\n");
    fflush(fp);
}

void TestBPlusTree::measurePerformance() {
    fflush(fp);
    int                             i;
    struct timespec                 t1, t2;
    my::BPlusTree<int, std::string> btree(BEST_ORDER);
    std::map<int, std::string>      map;
    std::vector<int>                keys;
    for (int j = 1; j <= treeSize[tmidx]; ++j) {
        keys.push_back(j);
    }
    fprintf(fp, "      ├── std::map:\n");
    i = 0;
    clock_gettime(CLOCK_REALTIME, &t1);
    for (size_t tidx = 0; tidx <= tmidx;) {
        if (map.size() == treeSize[tidx]) {
            clock_gettime(CLOCK_REALTIME, &t2);
            double insertTime = getTimeDifference(t2, t1);
            fprintf(fp, "      ├── size: %8zd  insert: %9.6lfs\n", map.size(), insertTime / 1000000);
            fflush(fp);
            tidx++;
        }
        map[keys[i++]] = std::string(STRING_SIZE, 'a');
    }
    map.clear();
    i = 0;
    fprintf(fp, "      ├── my::BPlusTree:\n");
    clock_gettime(CLOCK_REALTIME, &t1);
    for (size_t tidx = 0; tidx <= tmidx;) {
        if (btree.size() == treeSize[tidx]) {
            clock_gettime(CLOCK_REALTIME, &t2);
            double insertTime = getTimeDifference(t2, t1);
            fprintf(fp, "      ├── size: %8d  ncount: %7d  height: %2d  insert: %9.6lfs\n", btree.size(),
                    btree.ncount(), btree.height(), insertTime / 1000000);
            fflush(fp);
            tidx++;
        }
        btree.insert(keys[i++], std::string(STRING_SIZE, 'a'));
    }
    btree.clear();
    std::random_shuffle(keys.begin(), keys.end());
    i = 0;
    fprintf(fp, "      ├── std::map (random key):\n");
    clock_gettime(CLOCK_REALTIME, &t1);
    for (size_t tidx = 0; tidx <= tmidx;) {
        if (map.size() == treeSize[tidx]) {
            clock_gettime(CLOCK_REALTIME, &t2);
            double insertTime = getTimeDifference(t2, t1);
            fprintf(fp, "      ├── size: %8zd  insert: %9.6lfs\n", map.size(), insertTime / 1000000);
            fflush(fp);
            tidx++;
        }
        map[keys[i++]] = std::string(STRING_SIZE, 'a');
    }
    map.clear();
    i = 0;
    fprintf(fp, "      ├── my::BPlusTree (random key):\n");
    clock_gettime(CLOCK_REALTIME, &t1);
    for (size_t tidx = 0; tidx <= tmidx;) {
        if (btree.size() == treeSize[tidx]) {
            clock_gettime(CLOCK_REALTIME, &t2);
            double insertTime = getTimeDifference(t2, t1);
            fprintf(fp, "      ├── size: %8d  ncount: %7d  height: %2d  insert: %9.6lfs\n", btree.size(),
                    btree.ncount(), btree.height(), insertTime / 1000000);
            fflush(fp);
            tidx++;
        }
        btree.insert(keys[i++], std::string(STRING_SIZE, 'a'));
    }
    fprintf(fp, "      └── finished\n");
    fflush(fp);
}

double TestBPlusTree::testInsert(my::BPlusTree<int, std::string>& btree) {
    double                             insertTime = 0;
    struct timespec                    t1, t2;
    std::uniform_int_distribution<int> dist;
    std::random_device                 rd;
    std::default_random_engine         rng{ rd() };
    for (int i = 0; i < REPEAT; ++i) {
        int rand = dist(rng);
        clock_gettime(CLOCK_REALTIME, &t1);
        bool flag = btree.insert(rand, std::string(STRING_SIZE, 'a'));
        clock_gettime(CLOCK_REALTIME, &t2);
        if (flag) {
            insertTime += getTimeDifference(t2, t1);
            btree.erase(rand);
        }
    }
    return insertTime / REPEAT;
}

double TestBPlusTree::testErase(my::BPlusTree<int, std::string>& btree) {
    double                             eraseTime;
    struct timespec                    t1, t2;
    std::uniform_int_distribution<int> dist{ 0, (int)btree.size() - 1 };
    std::random_device                 rd;
    std::default_random_engine         rng{ rd() };
    for (int i = 0; i < REPEAT; ++i) {
        int  rand = dist(rng);
        auto it   = btree.begin();
        while (rand--) {
            ++it;
        }
        int         key  = it->first;
        std::string data = it->second;
        clock_gettime(CLOCK_REALTIME, &t1);
        btree.erase(key);
        clock_gettime(CLOCK_REALTIME, &t2);
        eraseTime += getTimeDifference(t2, t1);
        btree.insert(key, data);
    }
    return eraseTime / REPEAT;
}