struct RaftAddr {
    1: i32 ip,
    2: i16 port
}

struct RequestVoteParams {
    1: i32 term,
    2: RaftAddr candidateId,
    3: i32 lastLogIndex,
    4: i32 LastLogTerm
}

struct RequestVoteResult {
    1: i32 term,
    2: bool voteGranted
}

struct LogEntry {

}


struct AppendEntriesParams {
    1: i32 term,
    2: RaftAddr leaderId,
    3: i32 prevLogIndex,
    4: i32 prevLogTerm,
    5: list<LogEntry> entries,
    6: i32 leaderCommit
}

struct AppendEntriesResult {
    1: i32 term,
    2: bool success
}

service RaftRPC {
    RequestVoteResult requestVote(1: RequestVoteParams params),

    AppendEntriesResult appendEntries(1: AppendEntriesParams params)
}
