#include <stdio.h>
#include <fstream>
#include <iostream>

#include "KeyFinder.h"
#include "AddressUtil.h"
#include "util.h"
#include "secp256k1.h"
#include "CmdParse.h"
#include "Logger.h"
#include "ConfigFile.h"
#include "usage.h"
#include "argParser.h"

#include "DeviceManager.h"

#ifdef BUILD_CUDA
#include "CudaKeySearchDevice.h"
#endif

#ifdef BUILD_OPENCL
#include "CLKeySearchDevice.h"
#endif

typedef struct RunConfig {
    // startKey is the first key. We store it so that if the --continue
    // option is used, the correct progress is displayed. startKey and
    // nextKey are only equal at the very beginning. nextKey gets saved
    // in the checkpoint file.
    secp256k1::uint256 startKey = 1;
    secp256k1::uint256 nextKey = 1;

    // The last key to be checked
    secp256k1::uint256 endKey = secp256k1::N - 1;

    uint64_t statusInterval = 1800;
    uint64_t checkpointInterval = 60000;

    //unsigned int threads = 0;
    std::vector<unsigned int> threads;
    //unsigned int blocks = 0;
    std::vector<unsigned int> blocks;
    //unsigned int pointsPerThread = 0;
    std::vector<unsigned int> pointsPerThread;
    
    int compression = PointCompressionType::COMPRESSED;
 
    std::vector<std::string> targets;

    std::string targetsFile = "";

    std::string checkpointFile = "";

    //int device = 0;
    std::vector<unsigned int> devices;

    std::string resultsFile = "";

    uint64_t totalkeys = 0;
    unsigned int elapsed = 0;
    secp256k1::uint256 stride = 1;

    bool follow = false;
} RunConfig;

static RunConfig _config;

std::vector<DeviceManager::DeviceInfo> _devices;

void writeCheckpoint(secp256k1::uint256 nextKey);

static uint64_t _lastUpdate = 0;
static uint64_t _runningTime = 0;
static uint64_t _startTime = 0;

void resultCallback(KeySearchResult info);
void statusCallback(KeySearchStatus info);

/**
 Finds default parameters depending on the device
 */
typedef struct {
    int threads;
    int blocks;
    int pointsPerThread;
}DeviceParameters;

DeviceParameters getDefaultParameters(const DeviceManager::DeviceInfo &device);

static KeySearchDevice *getDeviceContext(DeviceManager::DeviceInfo &device, int blocks, int threads, int pointsPerThread);

static void printDeviceList(const std::vector<DeviceManager::DeviceInfo> &devices);

bool readAddressesFromFile(const std::string &fileName, std::vector<std::string> &lines);
void writeCheckpoint(secp256k1::uint256 nextKey);

void readCheckpointFile();
int run();
