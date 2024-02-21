Secure Database v0.1 (IN PROCESS)

This project was created just for fun and to put my cybersecurity knowledge into practice. It creates a command-line system to access a shared database securely. Each user should be able to have full read-write access into their own records, and can share certain records with other people, but there should be no way for any user A to read or write other users' records that have not been shared with A.

Note that this project is still in process. Therefore, the description below is subject to change. A list of expected upcoming changes can be found at the bottom of this document.

-------------------------------------------------------------------------

Secure Database is programmed in the C++ language. It uses the SQLite C library for database storage as well as the Crypto++ library for hashing, data encryption, and data decryption. The data is stored on a local machine; this project does not currently have network capabilities. Once the initial project is fully complete, I may add network capabilities as a separate secondary project.

Each client logs in using their username and password, both of which are stored hashed in the database. Once a client is logged in, they can access their stored data, edit it, and create new data records. Each record is stored under a given name, and thus the record-storage database table only requires two fields for the actual user-accessed data: "name" and "record", although other fields are present to keep track of record IDs, sharing permissions, etc.

Each user's password is used to generate their main encryption key. To ensure security, the password is stored double-hashed using a secure hashing algorithm: the first hash is used to generate the key, and the second hash is what is stored and visible in the database.

-------------------------------------------------------------------------

The command-line interface is as follows:

* Executable: "securedb"
  * Upon running the executable, the program will ask for the user's username and password.
* read NAME : decrypts and prints the contents of record NAME
* write NAME NEW_CONTENT : deletes the contents of NAME and replaces it with NEW_CONTENT. Creates NAME if it doesn't already exist.
* delete NAME : deletes NAME

Upcoming command-line features
* share NAME OTHER_USERNAME : allows OTHER_USERNAME read access to NAME's record
* help : print help text explaining all commands

Other commands may be added later after minimum viable product is reached.

-------------------------------------------------------------------------

Expected upcoming additions (roughly in order):

* Design threat model and security scheme for secure sharing
* Add sharing functionality to the command-line interface
* If found to be necessary, implement sign() and verify() wrapper functions over Crypto++, for ease of use and to isolate the functionality that Secure Database will need. See cryptowrapper.h
* Add capabilities to class AuthenticatedDBUser as necessary. See dbmanager.h

-------------------------------------------------------------------------

Upcoming design decisions/questions:

Perhaps the biggest design question is how to *securely* enable sharing functionality. Suppose that Alice wants to share a record R with Eve. If all of Alice's records are encrypted with the same key, then if Eve knows how to decrypt and read R, she can decrypt all of Alice's records. Although the program should check Eve's permissions for any record file R_i, defense in depth mandates stronger protections for Alice's data in case Eve attempts to read program memory to access the shared decryption key.

A tentative security scheme to ensure secure sharing functionality follows:

To ensure security, two types of symmetric encryption keys are used: "record keys", which are individual to each record, and "master keys", which are individual to each user and are used to encrypt all of the record keys that the user holds. The master key is generated based off of the first hash H(pwd) of the user's salted password and is never stored in the database (the second hash, H(H(pwd)) is what is stored, so even if an attacker has unfettered access to the database, finding the master key would require breaking H). The record keys are securely generated for each individual record.

A table Keys will exist in the database. Upon creation of a new record, the corresponding record key is encrypted with the user's master key and is stored in the database, along with identifying information linking that record key to the user it belongs to. With this in mind, it should be possible for a user Alice to securely share a single record R with another user Eve, without compromising any other records, as follows:
* Alice looks up the record key K_r for her record R in the Keys table.
* Alice decrypts K_r using her master key (which only she knows).
* Alice sends the decrypted K_r to Eve.
* Eve encrypts K_r with her master key (which only she knows), and stores the encrypted K_r, along with her identifying and record information, in the Keys table.

In this scenario, neither Alice nor Eve see each other's master keys. Assuming the transaction is secure, this record sharing method should not reveal the information about any record other than R to Eve or any potential eavesdropping third party.

The design questions this scheme raises are as follows (more may be added):
* How to ensure the security and integrity of the K_r handoff and prevent any third parties from snooping?
* How to verify Alice and Eve's authenticity throughout the handoff?
* Since this program involves secure sharing of records among multiple users *on the same machine*, how does Eve re-encrypt K_r? If only one process (belonging to Alice) is being run on the machine, can one ensure that Eve's master key doesn't leak to Alice?

