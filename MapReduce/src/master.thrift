enum ResponseType {
    WAIT = 0,
    MAP_TASK,
    REDUCE_TASK,
    COMPLETED
}

struct TaskResponse {
    1: i32 id,
    2: ResponseType type,
    3: list<string> params,
    
    /*
     * For map tasks, resultNum should be set to the number of reduce tasks, and
     * for reduce tasks, resultNum should always be 1
     */
    4: i32 resultNum
}

struct TaskResult {
    1: i32 id,
    2: ResponseType type,
    3: list<string> rs_loc 
}

service Master {
    i32 assignId();
    
    TaskResponse assignTask(),
    
    void commitTask(1: TaskResult result)
}
