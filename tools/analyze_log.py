from datetime import datetime
import os

curDir = os.getcwd()
logDir = f'{curDir}/../logs'

log_objs = {
    'raft1': [f'{logDir}/raft1/raft1.INFO'],
    'raft2': [f'{logDir}/raft2/raft2.INFO'],
    'raft3': [f'{logDir}/raft3/raft3.INFO']
}


class OneLog:
    def __init__(self, line, objNo):
        # I20230711 00:37:41.649639 11342532 raft.cpp:104] Switch to follower!
        meta, info = line.split(']')
        meta = meta.split(' ')
        self.time = datetime.strptime(
            f'{meta[0][1:]} {meta[1]}', '%Y%m%d %H:%M:%S.%f')
        self.process = meta[-1]
        self.info = info.strip('\n')
        self.objNo = objNo

    def __repr__(self) -> str:
        return f'[{self.info}] {self.process} {self.time}'

    def __str__(self) -> str:
        return f'[{self.info}] {self.process} {self.time}'


logs = []
for key in log_objs.keys():
    for logf in log_objs[key]:
        with open(logf, 'r') as f:
            for line in f:
                elems = line.split(']')
                if len(elems) != 2:
                    continue
                logs.append(OneLog(line, key))

logs.sort(key=lambda x: x.time, reverse=False)

key2i = {}
for key in log_objs.keys():
    key2i[key] = len(key2i)

print(key2i) 
with open('logs.tsv', 'w') as f:
    for log in logs:
        line = [''] * len(key2i)
        line[key2i[log.objNo]] = str(log)
        f.write('\t'.join(line))
        f.write('\n')
        print(log)