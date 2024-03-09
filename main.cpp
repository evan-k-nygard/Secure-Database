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
    
    
    AuthenticatedDBUser manager;
    try {
        manager = std::move(AuthenticatedDBUser(uname, pwd));
        std::cout << "Successfully signed in. Hello, " << uname << "!\n";
    } catch(...) {
        std::cerr << "Failed to sign in\n";
        return 1;
    }

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

        // these variables are used only in the DELETE case
        // defining them here to avoid errors
        bool gotResponse = false;
        bool affirmDeletion = false;
        
        switch(type) {
            case QUIT:
                running = false;
                break;
            case READ:
                recordName = args[0];
                try {
                    record = manager.retrieve_record(recordName);
                    std::cout << "Record '" << recordName << "':\n--------\n" << record << "\n--------\n";
                } catch(std::exception& e) {
                    std::cerr << "Error reading record: " << e.what() << '\n';
                }
                break;
            case WRITE:
                recordName = args[0];
                recordContent = args[1];
                // TODO differentiate between when record exists vs it doesn't
                try { // if the record doesn't exist, create it
                    manager.create_record(recordName, recordContent);
                } catch(...) {
                    try { // if the record DOES exist, create_record throws
                        // an exception. Catch the exception and edit the
                        // already existing record instead
                        manager.edit_record(recordName, recordContent);
                        std::cout << "Record '" << recordName << "' written\n";
                    } catch(std::exception& e) {
                        std::cerr << "Error writing record: " << e.what() << '\n';
                    }
                }
                break;
            case DELETE:
                recordName = args[0];
                while(!gotResponse) {
                    std::cout << "Are you sure you want to delete?\n";
                    std::cout << "[y/n]: ";
                    std::string affirm;
                    std::getline(std::cin, affirm);
                    if(affirm == "y") {
                        gotResponse = true;
                        affirmDeletion = true;
                    } else if(affirm == "n") {
                        gotResponse = true;
                        affirmDeletion = false;
                    } else {
                        std::cout << "Sorry, please enter 'y' or 'n'.\n";
                    }
                }
                if(affirmDeletion) {
                    try {
                        manager.delete_record(recordName);
                    } catch(std::exception& e) {
                        std::cerr << "Error on deletion: " << e.what() << '\n';
                        continue;
                    }
                    std::cout << "Record '" << recordName << "' deleted\n";
                } else {
                    std::cout << "Canceling deletion\n";
                }
                break;
            case RECORDLIST:
                try {
                    std::vector<std::string> names = manager.get_record_names();
                    for(size_t i = 0; i < names.size(); i++) {
                        std::cout << names[i] << '\n';
                    }
                } catch(std::exception& e) {
                    std::cerr << "Error on retrieving record names: " << e.what() << '\n';
                }
                break;
            case SHARE:
                std::cout << "Sorry! This functionality has not yet been implemented.\n";
                break;
            case HELP:
                std::cout << "Some help text\n";
                break;
            default:
                assert(false); // we should never reach this point
                std::cerr << "Unrecognzied token\n";
        }
    
    }
    return 0;
}