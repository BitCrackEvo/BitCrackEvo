#include "main.h"


/**
* Callback to display the private key
*/
void resultCallback(KeySearchResult info)
{
    if(_config.resultsFile.length() != 0) {
        Logger::log(LogLevel::Info, "Found key for address '" + info.address + "'. Written to '" + _config.resultsFile + "'");

        std::string s = info.address + " " + info.privateKey.toString(16) + " " + info.publicKey.toString(info.compressed);
        util::appendToFile(_config.resultsFile, s);

        return;
    }

    std::string logStr = "Address:     " + info.address + "\n";
    logStr += "Private key: " + info.privateKey.toString(16) + "\n";
    logStr += "Compressed:  ";

    if(info.compressed) {
        logStr += "yes\n";
    } else {
        logStr += "no\n";
    }

    logStr += "Public key:  \n";

    if(info.compressed) {
        logStr += info.publicKey.toString(true) + "\n";
    } else {
        logStr += info.publicKey.x.toString(16) + "\n";
        logStr += info.publicKey.y.toString(16) + "\n";
    }

    Logger::log(LogLevel::Info, logStr);
}

/**
Callback to display progress
*/
void statusCallback(KeySearchStatus info)
{
    std::string speedStr;

    if(info.speed < 0.01) {
        speedStr = "< 0.01 MKey/s";
    } else {
        speedStr = util::format("%.2f", info.speed) + " MKey/s";
    }

    std::string totalStr = "(" + util::formatThousands(_config.totalkeys + info.total) + " total)";

    std::string timeStr = "[" + util::formatSeconds((unsigned int)((_config.elapsed + info.totalTime) / 1000)) + "]";

    std::string usedMemStr = util::format((info.deviceMemory - info.freeMemory) /(1024 * 1024));

    std::string totalMemStr = util::format(info.deviceMemory / (1024 * 1024));

    std::string targetStr = util::format(info.targets) + " target" + (info.targets > 1 ? "s" : "");


    // Fit device name in 16 characters, pad with spaces if less
    std::string devName = info.deviceName.substr(0, 16);
    devName += std::string(16 - devName.length(), ' ');

    const char *formatStr = NULL;

    if(_config.follow) {
        formatStr = "%s %s/%sMB | %s %s %s %s\n";
    } else {
        formatStr = "\r%s %s / %sMB | %s %s %s %s";
    }

    printf(formatStr, devName.c_str(), usedMemStr.c_str(), totalMemStr.c_str(), targetStr.c_str(), speedStr.c_str(), totalStr.c_str(), timeStr.c_str());

    if(_config.checkpointFile.length() > 0) {
        uint64_t t = util::getSystemTime();
        if(t - _lastUpdate >= _config.checkpointInterval) {
            Logger::log(LogLevel::Info, "Checkpoint");
            writeCheckpoint(info.nextKey);
            _lastUpdate = t;
        }
    }
}

DeviceParameters getDefaultParameters(const DeviceManager::DeviceInfo &device)
{
    DeviceParameters p;
    p.threads = 256;
    p.blocks = 32;
    p.pointsPerThread = 32;

    return p;
}

static KeySearchDevice *getDeviceContext(DeviceManager::DeviceInfo &device, int blocks, int threads, int pointsPerThread)
{
#ifdef BUILD_CUDA
    if(device.type == DeviceManager::DeviceType::CUDA) {
        return new CudaKeySearchDevice((int)device.physicalId, threads, pointsPerThread, blocks);
    }
#endif

#ifdef BUILD_OPENCL
    if(device.type == DeviceManager::DeviceType::OpenCL) {
        return new CLKeySearchDevice(device.physicalId, threads, pointsPerThread, blocks);
    }
#endif

    return NULL;
}

/*
+---+---------+--------+-------+-------------+-------------------+-------------+
| ID       GPU  Tragets  Memory         Speed               Total        Time  |
+---+---------+--------+-------+-------------+-------------------+-------------+
|  0  RTX 3090        1    9501 107.58 MKey/s  15,000,000,000,000  00:10:52:31 |
+---+---------+--------+-------+-------------+-------------------+-------------+
*/

static void printDeviceList(const std::vector<DeviceManager::DeviceInfo> &devices)
{
    for(int i = 0; i < devices.size(); i++) {
        printf("ID:     %d\n", devices[i].id);
        printf("Name:   %s\n", devices[i].name.c_str());
        printf("Memory: %lldMB\n", devices[i].memory / ((uint64_t)1024 * 1024));
        printf("Compute units: %d\n", devices[i].computeUnits);
        printf("\n");
    }
}

bool readAddressesFromFile(const std::string &fileName, std::vector<std::string> &lines)
{
    if(fileName == "-") {
        return util::readLinesFromStream(std::cin, lines);
    } else {
        return util::readLinesFromStream(fileName, lines);
    }
}

void writeCheckpoint(secp256k1::uint256 nextKey)
{
    std::ofstream tmp(_config.checkpointFile, std::ios::out);

    tmp << "start=" << _config.startKey.toString() << std::endl;
    tmp << "next=" << nextKey.toString() << std::endl;
    tmp << "end=" << _config.endKey.toString() << std::endl;
    tmp << "blocks=" << _config.blocks << std::endl;
    tmp << "threads=" << _config.threads << std::endl;
    tmp << "points=" << _config.pointsPerThread << std::endl;
    tmp << "compression=" << getCompressionString(_config.compression) << std::endl;
    tmp << "device=" << _config.device << std::endl;
    tmp << "elapsed=" << (_config.elapsed + util::getSystemTime() - _startTime) << std::endl;
    tmp << "stride=" << _config.stride.toString();
    tmp.close();
}

void readCheckpointFile()
{
    if(_config.checkpointFile.length() == 0) {
        return;
    }

    ConfigFileReader reader(_config.checkpointFile);

    if(!reader.exists()) {
        return;
    }

    Logger::log(LogLevel::Info, "Loading ' " + _config.checkpointFile + "'");

    std::map<std::string, ConfigFileEntry> entries = reader.read();

    _config.startKey = secp256k1::uint256(entries["start"].value);
    _config.nextKey = secp256k1::uint256(entries["next"].value);
    _config.endKey = secp256k1::uint256(entries["end"].value);

    if(_config.threads == 0 && entries.find("threads") != entries.end()) {
        _config.threads = util::parseUInt32(entries["threads"].value);
    }
    if(_config.blocks == 0 && entries.find("blocks") != entries.end()) {
        _config.blocks = util::parseUInt32(entries["blocks"].value);
    }
    if(_config.pointsPerThread == 0 && entries.find("points") != entries.end()) {
        _config.pointsPerThread = util::parseUInt32(entries["points"].value);
    }
    if(entries.find("compression") != entries.end()) {
        _config.compression = parseCompressionString(entries["compression"].value);
    }
    if(entries.find("elapsed") != entries.end()) {
        _config.elapsed = util::parseUInt32(entries["elapsed"].value);
    }
    if(entries.find("stride") != entries.end()) {
        _config.stride = util::parseUInt64(entries["stride"].value);
    }

    _config.totalkeys = (_config.nextKey - _config.startKey).toUint64();
}

int run()
{
    if(_config.device < 0 || _config.device >= _devices.size()) {
        Logger::log(LogLevel::Error, "device " + util::format(_config.device) + " does not exist");
        return 1;
    }

    Logger::log(LogLevel::Info, "Compression: " + getCompressionString(_config.compression));
    Logger::log(LogLevel::Info, "Starting at: " + _config.nextKey.toString());
    Logger::log(LogLevel::Info, "Ending at:   " + _config.endKey.toString());
    Logger::log(LogLevel::Info, "Counting by: " + _config.stride.toString());

    try {

        _lastUpdate = util::getSystemTime();
        _startTime = util::getSystemTime();

        // Use default parameters if they have not been set
        DeviceParameters params = getDefaultParameters(_devices[_config.device]);

        if(_config.blocks == 0) {
            _config.blocks = params.blocks;
        }

        if(_config.threads == 0) {
            _config.threads = params.threads;
        }

        if(_config.pointsPerThread == 0) {
            _config.pointsPerThread = params.pointsPerThread;
        }

        // Get device context
        KeySearchDevice *d = getDeviceContext(_devices[_config.device], _config.blocks, _config.threads, _config.pointsPerThread);

        KeyFinder f(_config.nextKey, _config.endKey, _config.compression, d, _config.stride);

        f.setResultCallback(resultCallback);
        f.setStatusInterval(_config.statusInterval);
        f.setStatusCallback(statusCallback);

        f.init();

        if(!_config.targetsFile.empty()) {
            f.setTargets(_config.targetsFile);
        } else {
            f.setTargets(_config.targets);
        }

        f.run();

        delete d;
    } catch(KeySearchException ex) {
        Logger::log(LogLevel::Info, "Error: " + ex.msg);
        return 1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    bool optCompressed = false;
    bool optUncompressed = false;
    bool listDevices = false;
    bool optShares = false;
    bool optThreads = false;
    bool optBlocks = false;
    bool optPoints = false;

    uint32_t shareIdx = 0;
    uint32_t numShares = 0;

    printf("+-----------------------------------------------------------------+\n");
    printf("|                        BitCrackEvo V0.02                        |\n");
    printf("+-----------------------------------------------------------------+\n");

    // Catch --help or -h first
    for(int i = 1; i < argc; i++) {
        if(std::string(argv[i]) == "--help" || std::string(argv[i]) == "-h") {
            usage();
            return 0;
        }
    }

    // Check for supported devices
    try {
        _devices = DeviceManager::getDevices();

        if(_devices.size() == 0) {
            Logger::log(LogLevel::Error, "No devices available");
            return 1;
        }
    } catch(DeviceManager::DeviceManagerException ex) {
        Logger::log(LogLevel::Error, "Error detecting devices: " + ex.msg);
        return 1;
    }

    // Check for arguments
    if(argc == 1) {
        usage();
        return 0;
    }


    CmdParse parser;
    parser.add("-d", "--device", true);
    parser.add("-t", "--threads", true);
    parser.add("-b", "--blocks", true);
    parser.add("-p", "--points", true);
    parser.add("-d", "--device", true);
    parser.add("-c", "--compressed", false);
    parser.add("-u", "--uncompressed", false);
    parser.add("", "--compression", true);
    parser.add("-i", "--in", true);
    parser.add("-o", "--out", true);
    parser.add("-f", "--follow", false);
    parser.add("", "--list-devices", false);
    parser.add("", "--keyspace", true);
    parser.add("", "--continue", true);
    parser.add("", "--share", true);
    parser.add("", "--stride", true);

    try {
        parser.parse(argc, argv);
    } catch(std::string err) {
        Logger::log(LogLevel::Error, "Error: " + err);
        return 1;
    }

    std::vector<OptArg> args = parser.getArgs();

    for(unsigned int i = 0; i < args.size(); i++) {
        OptArg optArg = args[i];
        std::string opt = args[i].option;

        try {
            if(optArg.equals("-t", "--threads")) {
                _config.threads = util::parseUInt32(optArg.arg);
                optThreads = true;
            } else if(optArg.equals("-b", "--blocks")) {
                _config.blocks = util::parseUInt32(optArg.arg);
                optBlocks = true;
            } else if(optArg.equals("-p", "--points")) {
                _config.pointsPerThread = util::parseUInt32(optArg.arg);
                optPoints = true;
            } else if(optArg.equals("-d", "--device")) {
                _config.device = util::parseUInt32(optArg.arg);
            } else if(optArg.equals("-c", "--compressed")) {
                optCompressed = true;
            } else if(optArg.equals("-u", "--uncompressed")) {
                optUncompressed = true;
            } else if(optArg.equals("", "--compression")) {
                _config.compression = parseCompressionString(optArg.arg);
            } else if(optArg.equals("-i", "--in")) {
                _config.targetsFile = optArg.arg;
            } else if(optArg.equals("-o", "--out")) {
                _config.resultsFile = optArg.arg;
            } else if(optArg.equals("", "--list-devices")) {
                listDevices = true;
            } else if(optArg.equals("", "--continue")) {
                _config.checkpointFile = optArg.arg;
            } else if(optArg.equals("", "--keyspace")) {
                secp256k1::uint256 start;
                secp256k1::uint256 end;

                parseKeyspace(optArg.arg, start, end);

                if(start.cmp(secp256k1::N) > 0) {
                    throw std::string("argument is out of range");
                }
                if(start.isZero()) {
                    throw std::string("argument is out of range");
                }

                if(end.cmp(secp256k1::N) > 0) {
                    throw std::string("argument is out of range");
                }

                if(start.cmp(end) > 0) {
                    throw std::string("Invalid argument");
                }

                _config.startKey = start;
                _config.nextKey = start;
                _config.endKey = end;
            } else if(optArg.equals("", "--share")) {
                if(!parseShare(optArg.arg, shareIdx, numShares)) {
                    throw std::string("Invalid argument");
                }
                optShares = true;
            } else if(optArg.equals("", "--stride")) {
                try {
                    _config.stride = secp256k1::uint256(optArg.arg);
                } catch(...) {
                    throw std::string("invalid argument: : expected hex string");
                }

                if(_config.stride.cmp(secp256k1::N) >= 0) {
                    throw std::string("argument is out of range");
                }

                if(_config.stride.cmp(0) == 0) {
                    throw std::string("argument is out of range");
                }
            } else if(optArg.equals("-f", "--follow")) {
                _config.follow = true;
            }

        } catch(std::string err) {
            Logger::log(LogLevel::Error, "Error " + opt + ": " + err);
            return 1;
        }
    }

    if(listDevices) {
        printDeviceList(_devices);
        return 0;
    }

    // Verify device exists
    if(_config.device < 0 || _config.device >= _devices.size()) {
        Logger::log(LogLevel::Error, "device " + util::format(_config.device) + " does not exist");
        return 1;
    }

    // Parse operands
    std::vector<std::string> ops = parser.getOperands();

    // If there are no operands, then we must be reading from a file, otherwise
    // expect addresses on the commandline
    if(ops.size() == 0) {
        if(_config.targetsFile.length() == 0) {
            Logger::log(LogLevel::Error, "Missing arguments");
            usage();
            return 1;
        }
    } else {
        for(unsigned int i = 0; i < ops.size(); i++) {
            if(!Address::verifyAddress(ops[i])) {
                Logger::log(LogLevel::Error, "Invalid address '" + ops[i] + "'");
                return 1;
            }
            _config.targets.push_back(ops[i]);
        }
    }
    
    // Calculate where to start and end in the keyspace when the --share option is used
    if(optShares) {
        Logger::log(LogLevel::Info, "Share " + util::format(shareIdx) + " of " + util::format(numShares));
        secp256k1::uint256 numKeys = _config.endKey - _config.nextKey + 1;

        secp256k1::uint256 diff = numKeys.mod(numShares);
        numKeys = numKeys - diff;

        secp256k1::uint256 shareSize = numKeys.div(numShares);

        secp256k1::uint256 startPos = _config.nextKey + (shareSize * (shareIdx - 1));

        if(shareIdx < numShares) {
            secp256k1::uint256 endPos = _config.nextKey + (shareSize * (shareIdx)) - 1;
            _config.endKey = endPos;
        }

        _config.nextKey = startPos;
        _config.startKey = startPos;
    }

    // Check option for compressed, uncompressed, or both
    if(optCompressed && optUncompressed) {
        _config.compression = PointCompressionType::BOTH;
    } else if(optCompressed) {
        _config.compression = PointCompressionType::COMPRESSED;
    } else if(optUncompressed) {
        _config.compression = PointCompressionType::UNCOMPRESSED;
    }

    if(_config.checkpointFile.length() > 0) {
        readCheckpointFile();
    }

    return run();
}
