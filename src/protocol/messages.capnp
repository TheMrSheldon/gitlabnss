@0xbb29a6f04ade59c8;

using UserID = UInt32;

struct User {
    id @0 :UserID;
    username @1 :Text;
    name @2 :Text;
}

interface GitLabDaemon {
    getUserByID @0 (id :UserID) -> (user :User);
    getUserByName @1 (name :Text) -> (user :User);
}