import pycached

def set_test():
  mc = pycached.Client(["127.0.0.1:11211"])
  mc.set("hoge","aaa")

def get_test():
  mc = pycached.Client(["127.0.0.1:11211"])
  result = mc.get("hoge")
  assert(result == "aaa")

def set_many_test():
  from time import time
  mc = pycached.Client(["127.0.0.1:11211"])
  begin = time()
  for i in range(0, 1000):
    mc.set(str(i), str(i*i))
  elapsed = time() - begin
  print str(1000.0/elapsed) + "qps"
  assert(False)

def get_many_test():
  from time import time
  mc = pycached.Client(["127.0.0.1:11211"])
  begin = time()
  for i in range(0, 1000):
    assert(int(mc.get(str(i))) == i*i)
  elapsed = time() - begin
  print str(1000.0/elapsed) + "qps"
  #assert(False)

def cas_test():
  from time import time
  mc = pycached.Client(["127.0.0.1:11211"])
  mc.set("hoge","fuga")
  assert(mc.gets("hoge") == "fuga")
  mc.cas("hoge","auau")
  assert(mc.get("hoge") == "auau")
  mc.cas("hoge","bubu")
  assert(mc.get("hoge") == "auau")
