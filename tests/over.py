import os
count = 0
with open("op.log", "r") as fin:
    for line in fin:
    	count += 1
with open("last.log", "w") as fout:
    fout.write(str(count+1) + "\n")
