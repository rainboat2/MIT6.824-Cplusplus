typedef i32 TermId;

struct Host {
    1: string ip,
    2: i16 port
}

enum ServerState {
    FOLLOWER,
    CANDIDAE,
    LEADER
}

struct RequestVoteParams {
    1: TermId term,
    2: Host candidateId,
    3: i32 lastLogIndex,
    4: TermId LastLogTerm
}

struct RequestVoteResult {
    1: TermId term,
    2: bool voteGranted
}

struct LogEntry {
    1: TermId term,
    2: string command,
    3: i32 index
}


struct AppendEntriesParams {
    1: TermId term,
    2: Host leaderId,
    3: i32 prevLogIndex,
    4: TermId prevLogTerm,
    5: list<LogEntry> entries,
    6: i32 leaderCommit
}

struct AppendEntriesResult {
    1: TermId term,
    2: bool success
}

/*
 * record the state of raft server, only used for test
 */
struct RaftState {
    1: TermId currentTerm,
    2: Host votedFor,
    3: i32 commitIndex,
    4: i32 lastApplied,
    5: ServerState state,
    6: list<Host> peers,
    7: list<LogEntry> logs
}

struct StartResult {
    1: i32 expectedLogIndex,
    2: TermId term,
    3: bool isLeader
}

enum PutOp {
    PUT, APPEND
}

enum KVStatus {
    OK, ERR_NO_KEY, ERR_WRONG_LEADER
}

struct PutAppendParams {
    1: string key,
    2: string value,
    3: PutOp op
}

struct PutAppenRely {
    1: KVStatus status;
}

struct GetParams {
    1: string key
}

struct GetReply {
    1: KVStatus status
    2: string value
}

service Raft {
    RequestVoteResult requestVote(1: RequestVoteParams params);

    AppendEntriesResult appendEntries(1: AppendEntriesParams params);

    RaftState getState();

    StartResult start(1: string command); 
}

service KVRaft extends Raft{
    PutAppenRely putAppend(1: PutAppendParams params);

    GetReply get(1: GetParams params);
}
