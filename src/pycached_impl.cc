#ifdef NDEBUG
#include <boost/python.hpp>
#endif
#include <string>
#include <stdint.h>
#include <alloca.h>
#include <stdlib.h>
#include <memory.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <netinet/in.h> /* sockaddr_in */
#include <netdb.h> /* gethostbyname */
#include <time.h>
#include <sys/socket.h> /* socket(), bind(), listen(), accept(), recv() */
#include <errno.h> /* errno */
#include <arpa/inet.h>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <boost/unordered_map.hpp>
#include <boost/optional.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/range/value_type.hpp>
class SocketException : public std::runtime_error {
public:
    SocketException(const std::string& cause)
     : std::runtime_error("cause: " + cause) {}
};
class MemcachedException : public std::runtime_error {
public:
    MemcachedException(const std::string& cause)
     : std::runtime_error("cause: " + cause) {}
};

class Client
{
private:
  typedef boost::unordered_map<std::string, uint64_t> unique_map;
  //typedef std::vector<char> raw_data;
  typedef std::string raw_data;
  struct string_peace{
    const char* head_;
    uint32_t length_;
    string_peace(char* h, uint32_t l):head_(h), length_(l){}
    string_peace(char* h):head_(h), length_(strlen(h)){}
    string_peace(std::string str):head_(str.data()), length_(str.length()){}
    bool valid()const{
      return head_ != NULL;
    }
    operator std::string()const{
      return std::string(head_, length_);
    }
    void dump()const{
      std::string str(head_, length_);
      std::cerr << "string piece dump: " << str << std::endl;
    }
    string_peace():head_(NULL), length_(0){}

  private:
  };
  struct got_data{
    string_peace sp_;
    uint64_t unique_;
    got_data(string_peace s, uint64_t u)
      :sp_(s), unique_(u){}
  };
public:
  typedef std::vector<std::string> host_list;
  Client(const host_list hostlist){
    const std::string& host = hostlist[0];
    int delimiter_position = 0;
    for (size_t i=0; i < host.size(); ++i){
      if(host[i] == ':'){delimiter_position = i;}
    }
    if(delimiter_position == 0){throw MemcachedException("invalid address");}

    const uint32_t port_number = string_to_int(&host.data()[delimiter_position+1]);
    const std::string host_name(host.data(), delimiter_position);
    if(0xffffff < port_number){
      throw MemcachedException("invalid port number");
    }
    fd_ = socket(AF_INET, SOCK_STREAM, 0);

    if(::fcntl(fd_, F_SETFL, O_NONBLOCK) < 0){
      throw SocketException("set NONBLOCK");
    }
    sockaddr_in memcached;
    bzero(&memcached, sizeof(sockaddr_in)); /* zero clear struct */

    memcached.sin_family = AF_INET;
    memcached.sin_addr.s_addr = inet_addr(host_name.data());
    memcached.sin_port = htons(static_cast<uint16_t>(port_number));

    while(true){
      const int result = connect(fd_, (sockaddr*)&memcached, sizeof(sockaddr_in));
      //perror("socket");
      if(result == 0){
        //std::cerr  << "connect ok:" << std::endl;
        break;
      }else if(result == -1 && errno == 114){
        int wait_result = this->wait_for_timeout(fd_, 1000);
        if(wait_result == false){
          throw SocketException("could not connect. " + host);
        }else{
          //std::cerr << "connect: " << std::endl;
          return;
        }
      }else if(result == -1 && errno != 115){
        throw SocketException("could not connect. " + host);
      }
    }
  }
  raw_data get(const std::string& key){
    const uint32_t length = strlen("get \r\n") + key.length();
    char* const query = (char*)alloca(length);
    const uint32_t query_length =
      snprintf(query, length+1, "get %s\r\n", key.c_str());
    assert(length == query_length);
    //std::cerr << "query:" << query <<
    //" length:" << length <<
    //" query_len: " << query_length << std::endl;
    this->write(query, query_length);

    char* buffer=(char*)alloca(first_buffer);
    const char* const buffer_in_stack = buffer;
    got_data value = get_impl(&buffer);

    if(value.sp_.valid()){
      buffer[value.sp_.length_] = '\0';
      raw_data result(value.sp_.head_, &value.sp_.head_[value.sp_.length_]);
      if(buffer != buffer_in_stack){
        free(buffer);
      }
      return result;
    }else{
      if(buffer != buffer_in_stack){
        free(buffer);
      }
      return raw_data();
    }
  }
  raw_data gets(const std::string& key){
    const uint32_t length = strlen("gets \r\n") + key.length();
    char* const query = (char*)alloca(length+1);
    const uint32_t query_length =
      snprintf(query, length + 1, "gets %s\r\n", key.c_str());
    assert(length == query_length);
    //std::cout << "length :" << length << std::endl;
    //std::cout << "query :" << query << std::endl;
    this->write(query, query_length);

    char* buffer=(char*)alloca(first_buffer);
    const char* const buffer_in_stack = buffer;
    got_data value = get_impl(&buffer);

    if(value.sp_.valid()){
      buffer[value.sp_.length_] = '\0';
      raw_data result(value.sp_.head_, &value.sp_.head_[value.sp_.length_]);
      unique_numbers.erase(key);
      unique_numbers.insert(std::make_pair(key, value.unique_));
      if(buffer != buffer_in_stack){
        free(buffer);
      }
      return result;
    }else{
      if(buffer != buffer_in_stack){
        free(buffer);
      }
      return raw_data();
    }
  }
  void set(const std::string& key, const std::string& value){
    this->_store("set", key.data(), value.data(), false);
  }
  int add(const std::string& key, const std::string& value){
    return this->_store("add", key.data(), value.data(), false);
  }
  int cas(const std::string& key, const std::string& value){
    return this->_store("cas", key.data(), value.data(), true);
  }
  ~Client(){
    close(fd_);
  }
private:
  static const uint32_t first_buffer = 2048;
  static const int HEADER = 0, RAW_DATA = 1, END_SUFFIX = 2, PARSE_OK = 3;
  inline
  static boost::optional<std::pair<string_peace, uint64_t> >
  get_rawdata(const char* buffer, int buf_len){
    int state = HEADER;
    int readed = 0;
    int length;
    uint64_t unique = 0;
    string_peace result;
    while(true){
      switch(state){
      case HEADER:{
        if(strncmp(&buffer[readed], "VALUE ", 6)==0){
          const char* ptr = &buffer[readed+6];
          while(*ptr != ' ')++ptr;  // skip keyname
          skip_space(&ptr); // skip space
          while('0' <= *ptr && *ptr <= '9')++ptr;  // skip flag
          assert(*ptr == ' '); ++ptr;
          skip_space(&ptr);
          //printf("now ptr:%s\n", ptr);

          { // reading length
            std::pair<uint64_t,int> result = read_integer_from_string(ptr);
            length = result.first;
            ptr += result.second;
          }
          skip_space(&ptr);
          //printf("before h:%d\n", *ptr);
          { // read unique number
            if(*ptr != '\r' && *ptr != '\n'){
              std::pair<uint64_t,int> result = read_integer_from_string(ptr);
              unique = result.first;
              ptr += result.second;
            }
          }
          skip_space(&ptr);
          //printf("after:%d\n", *ptr);
          { // end line
            if(*ptr == '\r')++ptr;
            if(*ptr == '\n')++ptr;
            if(*ptr == '\r')++ptr;
          }
          readed += ptr - buffer;
          //std::cout << "readed " << readed << std::endl;
          state = RAW_DATA;
        }else if (strncmp(&buffer[readed], "END", 3) == 0){
          std::pair<string_peace, uint64_t> result =
            std::make_pair(string_peace(), 0);

          return result;
        }else{
          //std::cout << "invalid buffer :" << buffer << std::endl;
          return NULL;
        }
      }
      case RAW_DATA:{
        const char* ptr = &buffer[readed];
        if(buf_len <= readed + length){ // buffer length not yet growed
          return NULL; // read more
        }else{
          result.head_ = ptr;
          result.length_ = length;
          ptr += length;
          if(*ptr == '\r') ++ptr;
          if(*ptr == '\n') ++ptr;
          if(*ptr == '\r') ++ptr;
          readed += ptr - &buffer[readed];
          state = END_SUFFIX;
          //result.dump();
        }
        break;
      }
      case END_SUFFIX:{
        if (strncmp(&buffer[readed], "END", 3) == 0){
          state = PARSE_OK;
        }
        break;
      }
      case PARSE_OK:{
        //result.dump();
        //std::cerr << "parse done." << std::endl;
        return std::make_pair(result, unique);
      }
      }
    }
  }
  inline
  got_data get_impl(char** buffer){ // it returns pointer! must be free() in outside.
    int buffer_len = 2048;
    int buffer_rest = buffer_len;
    bool buffer_in_stack = true;
    int read_buffer = 0;
    while(true){
      int recv_len = ::read(fd_, &(*buffer)[read_buffer], buffer_rest);
      //perror("len");
      if(recv_len <= 0){
        if(recv_len == 0 || (errno != EINTR && errno != EAGAIN)) {
          ::shutdown(fd_, SHUT_RD);
          throw SocketException("cannot receive");
        }
        if(read_buffer == 0){ usleep(10); continue;}
        { // may end
          boost::optional<std::pair<string_peace, uint64_t> > result =
            get_rawdata(*buffer, read_buffer);
          //printf("result head is %p\n", result->head_);
          if(result){
            return got_data(result->first, result->second);
          }
        }
      } else{
        read_buffer += recv_len;
        buffer_rest -= recv_len;
        if(buffer_rest == 0){
          buffer_len *= 2;
          if(buffer_in_stack){
            char* new_buffer = (char*)malloc(buffer_len);
            memcpy(buffer, new_buffer, buffer_len / 2);
            *buffer = new_buffer;
          }else{
            *buffer = (char*)realloc(buffer, buffer_len);
          }
        }
      }
    }
  }
  inline
  bool _store(const char* command, const char* key, const char* value, const bool casflag){
    uint64_t cas_unique = 0;
    if(casflag){
      unique_map::const_iterator it = unique_numbers.find(key);
      if(it == unique_numbers.end()){
        throw MemcachedException("not gets value");
      }else{
        cas_unique = it->second;
      }
    }
    const uint32_t valuelen = strlen(value);
    if(valuelen < 1024){ // TODO: this value should be optimized
      const int length = strlen(command) + strlen(key)
        + valuelen + strlen("  0 0 \r\n\r\n")
        + digits_in_decimal(valuelen) + (casflag ? digits_in_decimal(cas_unique) + 1: 0);
      char* const query = (char*)alloca(length + 1);
      const int query_length = casflag ?
        snprintf(query, length + 1,"cas %s 0 0 %u %lu\r\n%s\r\n", key, valuelen, cas_unique, value) :
        snprintf(query, length + 1,"%s %s 0 0 %u\r\n%s\r\n", command, key, valuelen, value);
      //printf("query is %s\n", query);
      assert(query_length == length);
      this->write(query, length);
    }else{ // directry send from buffer without copy
      const int length = strlen(command) + strlen(key)
        + strlen("  0 0 \r\n\r\n")
        + digits_in_decimal(valuelen) + (casflag ? digits_in_decimal(cas_unique) + 1: 0);
      char* const query = (char*)alloca(length + 1);
      const int query_length = casflag ?
        snprintf(query, length + 1,"cas %s 0 0 %u %lu\r\n", key, valuelen, cas_unique) :
        snprintf(query, length + 1,"%s %s 0 0 %u\r\n", command, key, valuelen);
      assert(query_length == length);
      this->write(query, length);
      this->write(value, valuelen);
      this->write("\r\n", 2);
    }
    //std::cerr << "wrote." << std::endl;
    char* const result_buffer = (char*)alloca(32); // "STORED\r\n" ,"EXISTS\r\n", "NOT_STORED\r\n" etc...
    memset(result_buffer, 0, 32);

    this->read_result(result_buffer, 32-1);
    //printf("result: %s\n", result_buffer);

    //std::cerr << "read." << result_buffer << std::endl;

    if(strncmp(result_buffer, "STORED", 6) == 0){
      return true;
    }else if((strncmp(result_buffer, "EXISTS", 6) == 0)
             || strncmp(result_buffer, "NOT_STORED", 6) == 0){
      return false;
    }else{
      throw SocketException("unexpected result " + std::string(result_buffer));
      //std::cerr << result_buffer << std::endl;
    }
  }
  inline
  void read_result(char* const buffer, uint32_t len){
    char* bufferp = buffer;
    int rest_len = len;
    while(true){
      int recv_len = ::read(fd_, bufferp, rest_len);
      //std::cerr << recv_len << " ";
      if(recv_len <= 0){
        if(recv_len == 0 || (errno != EINTR && errno != EAGAIN)) {
          ::shutdown(fd_, SHUT_RD);
          throw SocketException("cannot receive");
        }
        {
          const char* buff = buffer;
          while(buff != bufferp && *buff != '\n') ++buff;
          if(*buff == '\n') return;
        }
      } else if(rest_len == recv_len){
        break;
      } else{
        bufferp += recv_len;
        rest_len -= recv_len;
      }
    }
  }

  void write(const char* const buffer, const uint32_t length){
    int rest_len = length;
    const char* buffer_p = buffer;;
    while(0 < rest_len){
      int sent_len = ::write(fd_, buffer_p, rest_len);
      if(sent_len <= 0){
        if(sent_len == 0 || (errno != EINTR && errno != EAGAIN)) {
          ::shutdown(fd_, SHUT_RD);
          return;
        }
      } else if(rest_len == sent_len) {
        return; // successfully finish!
      } else {
        buffer_p += sent_len;
        rest_len -= sent_len;
      }
    }
  }
  inline
  static void skip_space(const char** const ptr){
    while(**ptr == ' ')++(*ptr);
  }
  inline
  static std::pair<uint64_t, int> read_integer_from_string(const char* ptr){
    const char* const head = ptr;
    while('0' <= *ptr && *ptr <= '9')++ptr;
    const int length = ptr - head;
    char* const str = (char*)alloca(length+1);
    memcpy(str, head, length);
    str[length] = '\0';
    return std::make_pair(boost::lexical_cast<uint64_t>(str), length);
  }
  inline
  static bool wait_for_timeout(int fd, int time_msec){
    const int epollfd = epoll_create(10);
    struct epoll_event ev, events[10];
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    ev.data.fd = fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
    }
    int nfds = epoll_wait(epollfd, events, 10, time_msec);
    //std::cout << "epolled: " << nfds << std::endl;
    if(nfds <= 0){return false;}
    else{return true;}
  }
  inline
  static std::size_t digits_in_decimal(uint64_t number){
    int digits = 0;
    while(0 < number){
      number = number / 10;
      ++digits;
    }
    return digits;
  }
#ifndef NDEBUG
public:
#endif
  inline
  static uint64_t string_to_int(const char* ptr){
#ifndef NDEBUG
    for(size_t i=0;i < strlen(ptr); ++i){
      assert('0' <= ptr[i] && ptr[i] <= '9');
    }
#endif
    uint64_t num = 0, length = strlen(ptr);
    for(size_t i=0; i < length; ++i){
      num *= 10;
      num += (ptr[i] - '0');
    }
    return num;
  }
private:
  int fd_;
  unique_map unique_numbers;
};

/* enabled only generating python binding!! */
#ifdef NDEBUG
// lvalue converter
template<typename T_>
class vector_to_pylist_converter {
public:
    typedef T_ native_type;

    static PyObject* convert(native_type const& v) {
        namespace py = boost::python;
        py::list retval;
        BOOST_FOREACH(typename boost::range_value<native_type>::type i, v)
        {
            retval.append(py::object(i));
        }
        return py::incref(retval.ptr());
    }
};

// rvalue converter
template<typename T_>
class pylist_to_vector_converter {
public:
    typedef T_ native_type;
    static void* convertible(PyObject* pyo) {
        if (!PySequence_Check(pyo))
            return 0;
        return pyo;
    }
    static void construct(PyObject* pyo, boost::python::converter::rvalue_from_python_stage1_data* data)
    {
        namespace py = boost::python;
        native_type* storage = new(reinterpret_cast<py::converter::rvalue_from_python_storage<native_type>*>(data)->storage.bytes) native_type();
        for (py::ssize_t i = 0, l = PySequence_Size(pyo); i < l; ++i){
            storage->push_back(
                py::extract<typename boost::range_value<native_type>::type>(
                    PySequence_GetItem(pyo, i)));
        }
        data->convertible = storage;
    }
};


long zeroes(boost::python::list l)
{
   return l.count(0);
}

BOOST_PYTHON_MODULE(_pycached_impl)
{
  using namespace boost::python;
  class_<Client>("Client",init<const Client::host_list&>())
    .def("set", &Client::set)
    .def("add", &Client::add)
    .def("get", &Client::get)
    .def("gets",&Client::gets)
    .def("cas", &Client::cas)
    ;
  def("zeroes", &zeroes);
  to_python_converter<Client::host_list, vector_to_pylist_converter<Client::host_list> >();
  converter::registry::push_back(
    &pylist_to_vector_converter<Client::host_list>::convertible,
    &pylist_to_vector_converter<Client::host_list>::construct,
    boost::python::type_id<Client::host_list>());
}
#endif
