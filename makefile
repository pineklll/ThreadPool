.PHONY: run wangcheng

promise_future: promise_future.cpp
	g++ -std=c++20 promise_future.cpp -o promise_future
	
run: promise_future
	./promise_future

wangcheng:
	echo "En zhe mei lai ma?"