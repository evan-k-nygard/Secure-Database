#include <iostream>
#include <string>
#include <cstring>
#include "dbmanager.h"
#include "cryptowrapper.h"
#include "parsecmd.h"

std::string get_input_wo_newline() {
    std::string with_nl;
    std::getline(std::cin, with_nl);
    std::string result = with_nl.substr(0, std::string::npos - 1);
    return result;
}

int main(int argc, char** argv) {

    // authenticate the user
    std::cout << "Username: ";
    std::string uname = get_input_wo_newline();
    std::cout << "Password: ";
    std::string pwd = get_input_wo_newline();
    
    AuthenticatedDBUser manager(uname, pwd);
    bool running = true;
    while(running) {
        std::cout << "> ";
        std::string input = get_input_wo_newline();
        CommandType type;
        CommandArgs args;
        try {
            Command parse(input);
            type = parse.get_type();
            args = parse.get_args();
        } catch(std::exception& e) {
            std::cerr << "Error: " << e.what() << '\n';
            continue;
        }
        std::string recordName;
        std::string recordContent;
        std::string record;
        switch(type) {
            case QUIT:
                running = false;
                break;
            case READ:
                recordName = args[0];
                record = manager.retrieve_record(recordName);
                std::cout << "Record '" << recordName << "':\n--------\n" << record << "\n--------\n";
                break;
            case WRITE:
                recordName = args[0];
                recordContent = args[1];
                // TODO differentiate between when record exists vs it doesn't
                try {
                    manager.create_record(recordName, recordContent);
                } catch(...) {
                    manager.edit_record(recordName, recordContent);
                }
                std::cout << "Record '" << recordName << "' written\n";
                break;
            case DELETE:
                recordName = args[0];
                // TODO write affirmative function and confirm
                manager.delete_record(recordName);
                std::cout << "Record '" << recordName << "' deleted\n";
                break;
            case SHARE:
                std::cout << "Sorry! This functionality has not yet been implemented.\n";
                break;
            case HELP:
                std::cout << "Some help text\n";
                break;
            default:
                assert(false); // we should never reach this point
                std::cout << "Unrecognzied token\n";
        }
    }
    return 0;
}