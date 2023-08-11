typedef i32 TermId;
typedef i32 LogId;
typedef i32 GID;      // raft group id
typedef i32 ShardId ; // raft shared id

struct Host {
    1: string ip,
    2: i16 port
}

enum ErrorCode {
    SUCCEED = 0,
    ERR_REQUEST_FAILD,
    ERR_NO_KEY,
    ERR_WRONG_LEADER,
    ERR_NO_SUCH_SHARD_CONFIG
}

/* ================= structs for raft =====================*/

enum ServerState {
    FOLLOWER,
    CANDIDAE,
    LEADER
}

struct RequestVoteParams {
    1: TermId term,
    2: Host candidateId,
    3: LogId lastLogIndex,
    4: TermId LastLogTerm,
    5: GID gid
}

struct RequestVoteResult {
    1: TermId term,
    2: bool voteGranted,
    3: ErrorCode code;
}

struct LogEntry {
    1: TermId term,
    2: string command,
    3: LogId index
}


struct AppendEntriesParams {
    1: TermId term,
    2: Host leaderId,
    3: LogId prevLogIndex,
    4: TermId prevLogTerm,
    5: list<LogEntry> entries,
    6: LogId leaderCommit,
    7: GID gid
}

struct AppendEntriesResult {
    1: TermId term,
    2: bool success,
    3: ErrorCode code
}

/*
 * record the state of raft server, only used for test
 */
struct RaftState {
    1: TermId currentTerm,
    2: Host votedFor,
    3: LogId commitIndex,
    4: LogId lastApplied,
    5: ServerState state,
    6: list<Host> peers,
    7: list<LogEntry> logs
}

struct StartResult {
    1: LogId expectedLogIndex,
    2: TermId term,
    3: bool isLeader,
    4: ErrorCode code
}

/* ================= structs for kvraft =====================*/

enum PutOp {
    PUT, APPEND
}

struct PutAppendParams {
    1: string key,
    2: string value,
    3: PutOp op,
    4: GID gid,
    5: ShardId sid
}

struct PutAppendReply {
    1: ErrorCode code;
}

struct GetParams {
    1: string key
    2: GID gid,
    3: ShardId sid
}

struct GetReply {
    1: ErrorCode code 
    2: string value
}

struct InstallSnapshotParams {
    1: TermId term,
    2: Host leaderId,
    3: LogId lastIncludedIndex,
    4: TermId lastIncludedTerm,
    5: i32 offset,
    6: binary data,
    7: bool done,
    8: GID gid
}

/* ================= structs for shardctrler =====================*/
struct JoinArgs {
    1: map<GID, list<Host>> servers;
}

struct JoinReply {
    1: bool wrongLeader;
    2: ErrorCode code;
}

struct LeaveArgs {
    1: list<GID> gids;
}

struct LeaveReply {
    1: bool wrongLeader;
    2: ErrorCode code;
}

struct MoveArgs {
    1: ShardId shard;
    2: GID gid;
}

struct MoveReply {
    1: bool wrongLeader;
    2: ErrorCode code;
}

struct QueryArgs {
    1: i32 configNum;
}

struct Config {
    1: i32 configNum;
    2: list<GID> shard2gid;
    3: map<GID, set<ShardId>> gid2shards;
}

struct QueryReply {
    1: bool wrongLeader;
    2: Config config;
    3: ErrorCode code;
}

/* ================= structs for sharedkv =====================*/

struct PullShardParams {
    1: ShardId id;
}

struct PullShardReply {
    2: ErrorCode code;
}

service Raft {
    RequestVoteResult requestVote(1: RequestVoteParams params);

    AppendEntriesResult appendEntries(1: AppendEntriesParams params);

    RaftState getState();

    StartResult start(1: string command); 

    TermId installSnapshot(1: InstallSnapshotParams params);
}

service KVRaft extends Raft{
    PutAppendReply putAppend(1: PutAppendParams params);

    GetReply get(1: GetParams params);
}

service ShardCtrler extends Raft {
    JoinReply join(1: JoinArgs jargs);

    LeaveReply leave(1: LeaveArgs largs);

    MoveReply move(1: MoveArgs margs);

    QueryReply query(1: QueryArgs qargs);
}

service ShardKVRaft extends KVRaft {
    PullShardReply pullShardParams(1: PullShardParams params);
}