from datetime import datetime
import os
import re

curDir = os.getcwd()
logDir = f'{curDir}/../logs/TestBasicAgree2B'
RAFT_NUM = 3

def log_list(path):
    rs = []
    for file in os.listdir(path):
        if file.find("INFO") == -1 or file.endswith("INFO"):
            continue
        # if not file.endswith("INFO"):
        #     continue
        
        rs.append(f'{path}/{file}')
    return rs;

log_objs = {}
for i in range(1, RAFT_NUM + 1):
    key = f'raft{i}'
    log_objs[key] = log_list(f'{logDir}/{key}')
# log_objs["raft"] = log_list(f'{logDir}/raft')


# I20230711 00:37:41.649639 11342532 raft.cpp:104] Switch to follower!
log_p = re.compile('[IWEF](.{24}) ([\d]+) ([\w.]+):([\d]+)\] (.*)')

class OneLog:
    def __init__(self, time, file, line, msg, objNo):
        self.time = datetime.strptime(time, '%Y%m%d %H:%M:%S.%f')
        self.file = file
        self.line = line
        self.msg = msg.strip('\n')
        self.objNo = objNo

    def __repr__(self) -> str:
        time = self.time.strftime('%H:%M:%S.%f')
        return f'[{self.msg}] {self.file}:{self.line} ({time})'

    def __str__(self) -> str:
        time = self.time.strftime('%H:%M:%S.%f')
        return f'[{self.msg}] {self.file}:{self.line} ({time})'


logs = []
for key in log_objs.keys():
    for logf in log_objs[key]:
        with open(logf, 'r') as f:
            for line in f:
                m = log_p.match(line)
                if m is None:
                    continue
                logs.append(OneLog(m[1], m[3], m[4], m[5], key))

logs.sort(key=lambda x: x.time, reverse=False)

key2i = {}
for key in log_objs.keys():
    key2i[key] = len(key2i)

with open('logs.tsv', 'w') as f:
    for log in logs:
        line = [''] * len(key2i)
        line[key2i[log.objNo]] = str(log)
        f.write('\t'.join(line))
        f.write('\n')