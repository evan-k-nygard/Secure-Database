Secure Database v0.1 (IN PROCESS)

This project was created just for fun and to put my cybersecurity knowledge into practice. It creates a command-line system to access a shared database securely. Each user should be able to have full read-write access into their own records, and can share certain records with other people, but there should be no way for any user A to read or write other users' records that have not been shared with A.

Note that this project is still in process. Therefore, the description below is subject to change. A list of expected upcoming changes can be found at the bottom of this document.

-------------------------------------------------------------------------

Secure Database is programmed in the C++ language. It uses the SQLite C library for database storage as well as the Crypto++ library for hashing, data encryption, and data decryption. The data is stored on a local machine; this project does not currently have network capabilities. Once the initial project is fully complete, I may add network capabilities as a separate secondary project.

Each client logs in using their username and password, both of which are stored hashed in the database. Once a client is logged in, they can access their stored data, edit it, and create new data records. Each record is stored under a given name, and thus the record-storage database table only requires two fields for the actual user-accessed data: "name" and "record", although other fields are present to keep track of record IDs, sharing permissions, etc.

All user records are encrypted based on their password. However, this requires a means to share data with other users (and thus decryption keys) without compromising this password. To do this, the password is stored double-hashed using a secure hashing algorithm: the first hash is used as the decryption key, and the second hash is what is stored and visible in the database.

-------------------------------------------------------------------------

The anticipated command-line interface is as follows:

* Executable: "securedb -u USERNAME -p PASSWORD -h"
  * -u and -p are both required to log in
  * -h prints out a list of commands, and explains the login procedure
* read NAME : decrypts and prints the contents of record NAME
* write NAME NEW_CONTENT : deletes the contents of NAME and replaces it with NEW_CONTENT. Creates NAME if it doesn't already exist.
* delete NAME : deletes NAME
* share NAME OTHER_USERNAME : allows OTHER_USERNAME read access to NAME's record
* help : same functionality as -h in executable

Other commands may be added later after minimum viable product is reached.

-------------------------------------------------------------------------

Expected upcoming additions (roughly in order):

- Finish and test the C++ wrapper over SQLite - class DBManager, used to implement all relevant database operations in a C++ style framework. See dbmanager.h
- Create and test encrypt(), decrypt(), and hash() wrapper functions over Crypto++, for ease of use and to isolate the functionality that Secure Database will need. sign() and verify() functions may also be added at this stage (see below for more on this)
- Create command line interface, adding and testing functionality incrementally:
--- Login functionality
--- Read/write functionality
--- Sharing functionality

-------------------------------------------------------------------------

Upcoming design decisions/questions:

Perhaps the biggest design question is how to *securely* enable sharing functionality. Suppose that Alice wants to share a record R with Eve. If all of Alice's records are encrypted with the same key, then if Eve knows how to decrypt and read R, she can decrypt all of Alice's records. Although the program should check Eve's permissions for any record file R_i, defense in depth mandates stronger protections for Alice's data in case Eve attempts to read program memory to access the shared decryption key.

My current hunches towards ensuring security in this regard is to:
(a) ensure unique keys for each record by building a composite cryptographic key based on multiple factors - for example, the record name, the record id, the password hash, the time of original creation, and so forth
(b) verify Eve's access to record R by using a private/public key signing mechanism.

Note, however, that (b) raises the problem of how to securely store the private key itself. This is a more difficult problem, especially since this program is shared on a local device - the database is accessible to multiple users.

