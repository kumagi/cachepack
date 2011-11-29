import _pycached_impl
import msgpack

class Client(_pycached_impl.Client):
  def set(self, key,value):
    serialized_value = msgpack.packb(value)
    return _pycached_impl.Client.set(key, serialized_value)
  def add(self, key,value):
    serialized_value = msgpack.packb(value)
    return _pycached_impl.Client.add(key, serialized_value)
  def cas(self, key,value):
    serialized_value = msgpack.packb(value)
    return _pycached_impl.Client.cas(key, serialized_value)
  def get(self, key):
    return msgpack.unpack(_pycached_impl.Client.get(key))
  def gets(self, key):
    return msgpack.unpack(_pycached_impl.Client.gets(key))
