#include <iostream>
#include "dbmanager.h"

void resetDatabase();
void resetUser1();
void resetUser2();

int main() {
    std::cout << "Resetting tests...\n";
    resetDatabase();
    resetUser1();
    resetUser2();
    std::cout << "Done!\n";
    return 0;
}

void resetDatabase() {
    DB db("runtests.db");
    db.prepared_query("drop table Keys", ArgumentList({}));
    db.prepared_query("drop table Records", ArgumentList({}));
    db.prepared_query("create table Keys(user varchar(256), record_name varchar(512), key varchar(2048));", ArgumentList({}));
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