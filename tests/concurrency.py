import logging
import threading
import time
import os

def read(name):
    logging.info("Thread %s: start reading", name)
    os.system("cat mount1/large.1MB.{}".format(name))
    logging.info("Thread %s: finish reading", name)

def write(name):
    logging.info("Thread %s: start writing", name)
    os.system("cp large.1M mount1/large.1MB.{}".format(name))
    logging.info("Thread %s: finish writing", name)

if __name__ == "__main__":

    logging.basicConfig(level=logging.INFO,
                        filename='concurrency.log',
                        filemode='w',
                        format= '%(asctime)s - %(pathname)s[line:%(lineno)d] - %(levelname)s: %(message)s'
                        )
    nfile = 10
    nwriter = 1
    nreader = 10
    for index in range(nfile):
        write(str(index))
    for index in range(nfile):
        read(str(index))
    for index in range(nfile):
        for i in range(nwriter):
            writer = threading.Thread(target=write, args=(index,))
            writer.start()
        for i in range(nreader):
            reader = threading.Thread(target=read, args=(index,))
            reader.start()
