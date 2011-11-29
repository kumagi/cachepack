import _pycached_impl
import msgpack
class Client(_pycached_impl.Client):
  def set(self, key,value):
    return _pycached_impl.Client.set(self, key, msgpack.packb(value))
  def add(self, key,value):
    return _pycached_impl.Client.add(self, key, msgpack.packb(value))
  def cas(self, key,value):
    return _pycached_impl.Client.cas(self, key, msgpack.packb(value))
  def get(self, key):
    return msgpack.unpackb(_pycached_impl.Client.get(self, key))
  def gets(self, key):
    return msgpack.unpackb(_pycached_impl.Client.gets(self, key))
