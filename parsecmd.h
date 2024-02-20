#include <vector>

#ifndef __PARSECMD_H
#define __PARSECMD_H

typedef enum { READ, WRITE, DELETE, SHARE, HELP, QUIT } CommandType;
typedef std::vector<std::string> CommandArgs;

class Command {
    private:
        CommandType type;
        CommandArgs args;
    public:
        Command(const std::string& cmd);
        ~Command();

        CommandType get_type();
        CommandArgs get_args();
};

std::string commandTypeToString(CommandType type);

#endif