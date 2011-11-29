CC=g++
OPTS=-g -std=c++0x
GTEST_DIR=/opt/google/gtest-1.6.0
GTEST_INC= -I$(GTEST_DIR)/include -I$(GTEST_DIR)
WARNS= -W -Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align \
		-Wwrite-strings -Wfloat-equal -Wpointer-arith -Wswitch-enum
NOTIFY=&& notify-send Test success! -i ~/themes/ok_icon.png || notify-send Test failed... -i ~/themes/ng_icon.png

target:test_pycached
test_pycached:pycached_test.o libgtest.a
	$(CC) $^ $(OPTS) $(WARNS) -o $@ -lpthread
	./$@ $(NOTIFY)
pycached_test.o:pycached_test.cc pycached_impl.cc
	$(CC) -c $< $(GTEST_INC) $(OPTS) $(WARNS) -o $@

# gtest
libgtest.a:
	$(CXX) ${GTEST_INC} -c ${GTEST_DIR}/src/gtest-all.cc -o gtest-all.o
	$(CXX) ${GTEST_INC} -c ${GTEST_DIR}/src/gtest_main.cc -o gtest_main.o
	ar -rv libgtest.a gtest-all.o gtest_main.o

.cc.o:
	$(CXX) $(GTEST_INC) $(OPTS) $< -o $@

clean:
	rm -f *.o
	rm -f *~

