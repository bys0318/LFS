import os
import sys
ops = []
count = 0
with open("last.log", "r") as fin:
    last_count = int(fin.read().strip('\n'))
with open("op.log", "r") as fin:
    for line in fin:
        count += 1
        ops.append((line.strip().split(',')[0], line.strip().split(',')[1]))
ops = ops[last_count-1:]
for op in ops:
    print(op[0], op[1])
command = sys.argv[1]
if command == "ls":
    if ops[-4][0] == "GETATTR" and ops[-3][0] == "OPENDIR" and ops[-2][0] == "READDIR" and ops[-1][0] == "RELEASEDIR":
        print("SUCCESS")
    else:
        print("FAIL")
elif command == "cat":
    if ops[-4][0] == "GETATTR" and ops[-3][0] == "OPEN" and ops[-2][0] == "READ" and ops[-1][0] == "RELEASE":
        print("SUCCESS")
    else:
        print("FAIL")
elif command == "chmod":
    if ops[-3][0] == "GETATTR" and ops[-2][0] == "CHMOD" and ops[-1][0] == "GETATTR":
        print("SUCCESS")
    else:
        print("FAIL")
elif command == "mv":
    if ops[-3][0] == "GETATTR" and ops[-2][0] == "ACCESS" and ops[-1][0] == "RENAME":
        print("SUCCESS")
    else:
        print("FAIL")
elif command == "cp":
    if ops[-9][0] == "GETATTR" and ops[-8][0] == "GETATTR" and ops[-7][0] == "OPEN" and ops[-6][0] == "OPEN" and ops[-5][0] == "GETXATTR" and ops[-4][0] == "GETATTR" and ops[-3][0] == "READ" and ops[-2][0] == "RELEASE" and ops[-1][0] == "RELEASE":
        print("SUCCESS")
    else:
        print("FAIL")
elif command == "rm":
    if ops[-2][0] == "ACCESS" and ops[-1][0] == "UNLINK":
        print("SUCCESS")
    else:
        print("FAIL")
with open("last.log", "w") as fout:
    fout.write(str(count+1) + "\n")

