import _pycached_impl
import msgpack

class Client(_pycached_impl.Client):
  def set(self, key,value):
    serialized_value = msgpack.packb(value)
    return _pycached_impl.Client.set(self, key, serialized_value)
  def add(self, key,value):
    serialized_value = msgpack.packb(value)
    return _pycached_impl.Client.add(self, key, serialized_value)
  def cas(self, key,value):
    serialized_value = msgpack.packb(value)
    return _pycached_impl.Client.cas(self, key, serialized_value)
  def get(self, key):
    return msgpack.unpackb(_pycached_impl.Client.get(self, key))
  def gets(self, key):
    return msgpack.unpackb(_pycached_impl.Client.gets(self, key))
