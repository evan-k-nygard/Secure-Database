#include <iostream>
#include "dbmanager.h"
#include "cryptowrapper.h"

// two users: test1, password test1pwd; test2, password test2pwd

/* Unit test function declarations */

// valid authentication is handled separately
int testInvalidAuthentication(const std::string& u, const std::string& p, int testNum);

int testValidRecordReading(AuthenticatedDBUser& user, std::string name, std::string expectedContent);
int testInvalidRecordReading(AuthenticatedDBUser& user, std::string name);

int testValidRecordCreation(AuthenticatedDBUser& user, std::string name);
int testInvalidRecordCreation(AuthenticatedDBUser& user, std::string name);

int testValidRecordEdit(AuthenticatedDBUser& user, std::string name, std::string edit);
int testInvalidRecordEdit(AuthenticatedDBUser& user, std::string name);

int testValidRecordDeletion(AuthenticatedDBUser& user, std::string name);
int testInvalidRecordDeletion(AuthenticatedDBUser& user, std::string name);

int testValidRecordListing(AuthenticatedDBUser& user, std::vector<std::string> expectedList);


void resetDatabase();
void resetUser1();
void resetUser2();

/* Run unit tests */
int main() {
    std::cout << "Resetting tests...\n";
    resetDatabase();
    resetUser1();
    resetUser2();
    std::cout << "Finished resetting tests.\n";
    std::cout << "---------------------------------------------------\n\n";

    std::cout << "Running first tests: logins\n";
    // confirm unsuccessful logins as invalid user w/ junk password
    if(testInvalidAuthentication("nonexistent", "pwd", 1) == 1) return 1;
    
    // confirm unsuccessful logins as valid user w/ incorect password
    if(testInvalidAuthentication("test1", "invalidpwd", 2) == 1) return 1;

    // confirm successful logins as two valid users
    AuthenticatedDBUser alice;
    AuthenticatedDBUser bob;
    try {
        alice = AuthenticatedDBUser("test1", "test1pwd", "runtests.db");
    } catch(std::exception& e) {
        std::cout << "Failed login test 3: " << e.what() << '\n';
        return 1;
    }

    try {
        bob = AuthenticatedDBUser("test2", "test2pwd", "runtests.db");
    } catch(std::exception& e) {
        std::cout << "Failed login test 4: " << e.what() << '\n';
        return 1;
    }


    std::cout << "Passed login tests\n";
    std::cout << "Running functionality tests\n";
    std::cout << "Functionality test 1: record reading\n";
    // confirm successful reading of existing records as users A and B
    // confirm unsuccesful reading of nonexistent records as users A and B
    // confirm user A unsuccessfully reads B's record
    // confirm user B unsuccessfully reads A's record
    if(testValidRecordReading(alice, "permanent1", "permanent1") == 1) return 1;
    if(testValidRecordReading(bob, "permanent2", "permanent2") == 1) return 1;
    if(testInvalidRecordReading(alice, "nonexistent") == 1) return 1;
    if(testInvalidRecordReading(bob, "nonexistent") == 1) return 1;
    if(testInvalidRecordReading(alice, "permanent2") == 1) return 1;
    if(testInvalidRecordReading(bob, "permanent1") == 1) return 1;

    if(testValidRecordListing(alice, std::vector<std::string>({"permanent1"})) == 1) return 1;
    if(testValidRecordListing(bob, std::vector<std::string>({"permanent2"})) == 1) return 1;

    std::cout << "Functionality test 2: record creation\n";
    // confirm successful creation of new records X, Y as users A and B respectively
    // confirm new records are accurately paired to their owners
    // confirm unsuccessful creation of existing records X, Y as users A and B
    if(testValidRecordCreation(alice, "A1") == 1) return 1;
    if(testValidRecordCreation(bob, "A2") == 1) return 1;
    if(testInvalidRecordCreation(alice, "A1") == 1) return 1;
    if(testInvalidRecordCreation(bob, "A2") == 1) return 1;

    // intendend functionality here may change. Right now, users cannot create
    // records that have the same name as another user's record
    // This may change; but for now, verify this functionality
    if(testValidRecordCreation(alice, "A2") == 1) return 1;
    if(testValidRecordCreation(bob, "A1") == 1) return 1;
    if(testValidRecordListing(alice, std::vector<std::string>({"permanent1", "A1", "A2"})) == 1) return 1;
    if(testValidRecordListing(bob, std::vector<std::string>({"permanent2", "A2", "A1"})) == 1) return 1;
    
    std::cout << "Functionality test 3: record editing\n";
    // confirm successful edits of records X, Y as users A and B
    // confirm unsuccessful edits of record X by user B, and record Y by user A
    // confirm unsuccessful edits of nonexistent records by both users
    if(testValidRecordEdit(alice, "A1", "new content") == 1) return 1;
    if(testValidRecordEdit(bob, "A2", "new content") == 1) return 1;
    if(testValidRecordEdit(alice, "A2", "other new content") == 1) return 1;
    if(testValidRecordEdit(bob, "A1", "other new content") == 1) return 1;
    if(testInvalidRecordEdit(alice, "nonexistent") == 1) return 1;
    if(testInvalidRecordEdit(bob, "nonexistent") == 1) return 1;

    std::cout << "Functionality test 4: record deletion\n";
    // confirm unsuccessful deletion of nonexistent records by users A and B
    // confirm unsuccessful deletion of record X by B, record Y by A
    // confirm successful deletion of record X by A, Y by B
    // confirm unsuccessful reading of record X by A, Y by B
    if(testInvalidRecordDeletion(alice, "nonexistent") == 1) return 1;
    if(testInvalidRecordDeletion(bob, "nonexistent") == 1) return 1;
    if(testValidRecordDeletion(alice, "A2") == 1) return 1;
    if(testValidRecordDeletion(bob, "A1") == 1) return 1;
    if(testValidRecordDeletion(alice, "A1") == 1) return 1;
    if(testValidRecordDeletion(bob, "A2") == 1) return 1;

    if(testValidRecordListing(alice, std::vector<std::string>({"permanent1"})) == 1) return 1;
    if(testValidRecordListing(bob, std::vector<std::string>({"permanent2"})) == 1) return 1;

    std::cout << "Functionality tests passed\n";
    std::cout << "All tests passed!\n";
    return 0;
}


/* Reset function definitions */
void resetDatabase() {
    DB db("runtests.db");
    db.prepared_query("drop table Keys", ArgumentList({}));
    db.prepared_query("drop table Records", ArgumentList({}));
    db.prepared_query("create table Keys(user varchar(640), record_name varchar(2048), record_identifier varchar(640), key varchar(2048));", ArgumentList({}));
    db.prepared_query("create table Records(id int primary key, owner int not null, name varchar(512), record varchar(4096), foreign key(owner) references Users(id))", ArgumentList({}));
    std::cout << "Successfully reset database\n";
}

void resetUser1() {
    AuthenticatedDBUser user1("test1", "test1pwd", "runtests.db");
    user1.create_record("permanent1", "permanent1");
    std::cout << "Successfully reset user 'test1'\n";
}

void resetUser2() {
    AuthenticatedDBUser user2("test2", "test2pwd", "runtests.db");
    user2.create_record("permanent2", "permanent2");
    std::cout << "Successfully reset user 'test2'\n";
}

/* Unit test function definitions */

int testInvalidAuthentication(const std::string& u, const std::string& p, int testNum) {
    // affirm that authentication fails when we enter an invalid uname/pwd combo
    try {
        AuthenticatedDBUser t1(u, p, "runtests.db");
    } catch(std::exception& e) {
        if(e.what() == std::string("Could not authenticate")) {
            return 0;
        } else {
            std::cout << "Alternative error on login test " << testNum << ": " << e.what() << '\n';
            return 1;
        }
    }
    std::cout << "Failed login test " << testNum << ": nonexistent user successfully authenticated\n";
    return 1;
}

int testValidRecordReading(AuthenticatedDBUser& user, std::string name, std::string expectedContent) {
    try {
        std::string content = user.retrieve_record(name);
        if(content != expectedContent) {
            std::cout << "Failed test: expected '" << expectedContent << "', got '" << content << "'\n";
            return 1;
        }
    } catch(const std::exception& e) {
        std::cout << "Failed test on exception: " << e.what() << '\n';
        return 1;
    }
    return 0;
}

int testInvalidRecordReading(AuthenticatedDBUser& user, std::string name) {
    try {
        user.retrieve_record(name);
    } catch(const std::exception& e) {
        return 0;
    }
    std::cout << "Failed test: did not catch expected exception\n";
    return 1;
}

int testValidRecordCreation(AuthenticatedDBUser& user, std::string name) {
    try {
        user.create_record(name, name);
    } catch(std::exception& e) {
        std::cout << "Failed record creation test: " << e.what() << '\n';
        return 1;
    }

    // verify successful record creation by reading it
    try {
        std::string test = user.retrieve_record(name);
        if(test != name) {
            user.delete_record(name);
            std::cout << "Failed record creation test: unexpected record content\n";
            return 1;
        }
    } catch(std::exception& e) {
        std::cout << "Failed record creation test on reading: " << e.what() << '\n';
        return 1;
    }
    return 0;
}

int testInvalidRecordCreation(AuthenticatedDBUser& user, std::string name) {
    try {
        user.create_record(name, name);
    } catch(std::exception& e) {
        return 0;
    }

    std::cout << "Failed record creation test: successfully created invalid record '" << name << "'\n";
    user.delete_record(name);
    return 1;
}

int testValidRecordEdit(AuthenticatedDBUser& user, std::string name, std::string edit) {
    try {
        user.edit_record(name, edit);
    } catch(std::exception& e) {
        std::cout << "Failed record editing test: " << e.what() << '\n';
        return 1;
    }

    // verify successful record editing
    try {
        if(user.retrieve_record(name) != edit) {
            user.delete_record(name);
            std::cout << "Failed record editing test: unexpected record content\n";
            return 1;
        }
    } catch(std::exception& e) {
        std::cout << "Failed record editing test on reading: " << e.what() << '\n';
        return 1;
    }
    return 0;
}

int testInvalidRecordEdit(AuthenticatedDBUser& user, std::string name) {
    try {
        user.edit_record(name, "this shouldn't work");
    } catch(std::exception& e) {
        return 0;
    }
    std::cout << "Failed record editing test: successfully edited invalid record\n";
    user.delete_record(name);
    return 1;
}

int testValidRecordDeletion(AuthenticatedDBUser& user, std::string name) {
    try {
        user.delete_record(name);
    } catch(std::exception& e) {
        std::cout << "Failed record deletion test: " << e.what() << '\n';
        return 1;
    }
    
    // verify deletion of record
    if(user.record_exists(name)) {
        std::cout << "Failed record deletion test: record still exists after deletion\n";
        return 1;
    } else {
        return 0;
    }
}

int testInvalidRecordDeletion(AuthenticatedDBUser& user, std::string name) {
    try {
        user.delete_record(name);
    } catch(std::exception& e) {
        return 0;
    }

    return 1;
}

int testValidRecordListing(AuthenticatedDBUser& user, std::vector<std::string> expectedList) {
    try {
        std::vector<std::string> test = user.get_record_names();
        if(test == expectedList) return 0;
        else {
            std::cout << "Failed record listing test: record list differs from expected record\n";
            return 1;
        }
    } catch(std::exception& e) {
        std::cout << "Failed record listing test: an exception was thrown: " << e.what() << '\n';
        return 1;
    }
}