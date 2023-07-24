
enum PutOp {
    PUT, APPEND
}

enum KVStatus {
    OK, ERR_NO_KEY, ERR_WRONG_LEADER
}

struct PutAppendArgs {
    1: string key,
    2: string value,
    3: PutOp op
}

struct PutAppenRely {
    1: KVStatus status;
}

struct GetArgs {
    1: string key
}

struct GetReply {
    1: KVStatus status
    2: string value
}

service KVRaft {
    PutAppenRely putAppend(1: PutAppendArgs args);

    GetReply get(1: GetArgs args);
}