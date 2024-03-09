#include <sstream>
#include <vector>
#include <iomanip>
#include <iostream>
#include "parsecmd.h"

std::string commandTypeToString(CommandType type) {
    switch(type) {
        case READ:
            return "read";
        case WRITE:
            return "write";
        case DELETE:
            return "delete";
        case SHARE:
            return "share";
        case RECORDLIST:
            return "list";
        case HELP:
            return "help";
        case QUIT:
            return "quit";
        default:
            assert(false); // should never get here
    }
}

Command::Command(const std::string& cmd) {
    std::istringstream parser(cmd);
    std::string token;

    int expectedArgs = 0;
    int seenArgs = 0;

    bool foundType = false;
    while(parser >> std::quoted(token)) {
        if(!foundType) {
            if(token == "read") {
                type = READ;
                expectedArgs = 1;
            } else if(token == "write") {
                type = WRITE;
                expectedArgs = 2;
            } else if(token == "delete") {
                type = DELETE;
                expectedArgs = 1;
            } else if(token == "share") {
                type = SHARE;
                expectedArgs = 2;
            } else if(token == "list") {
                type = RECORDLIST;
                expectedArgs = 0;
            } else if(token == "help") {
                type = HELP;
                expectedArgs = 0;
            } else if(token == "quit") {
                type = QUIT;
                expectedArgs = 0;
            } else {
                std::string e = "invalid command '";
                e += token + "'";
                throw std::runtime_error(e);
            }
            foundType = true;
        } else {
            seenArgs += 1;
            if(seenArgs > expectedArgs) {
                std::string e = "too many arguments for command '";
                e += commandTypeToString(type) + "'";
                throw std::runtime_error(e);
            }
            args.push_back(token);
        }
    }
    if(seenArgs < expectedArgs) {
        std::string e = "too few arguments for command '";
        e += commandTypeToString(type) + "'";
        throw std::runtime_error(e);
    }
}

Command::~Command() {}

CommandType Command::get_type() {
    return type;
}

CommandArgs Command::get_args() {
    return args;
}