sampleobjects = buffer_manager.o file_manager.o main.o

kdb : $(sampleobjects)
	     g++ -std=c++11 -o kdbtree $(sampleobjects)

sample_run : $(sampleobjects)
	     g++ -std=c++11 -o sample_run $(sampleobjects)

sample_run.o : sample_run.cpp
	g++ -std=c++11 -c sample_run.cpp

buffer_manager.o : buffer_manager.cpp
	g++ -std=c++11 -c buffer_manager.cpp

file_manager.o : file_manager.cpp
	g++ -std=c++11 -c file_manager.cpp

main.o : main.cpp
	g++ -std=c++11 -c main.cpp

clean :
	rm -f *.o
	rm -f sample_run
	rm -f temp.txt

clear :
	rm -f output.txt
	rm -f temp.txt

run:
	./kdbtree /mnt/d/sem6/col362/assign/a3/kdb-tree/query.txt 2 /mnt/d/sem6/col362/assign/a3/kdb-tree/output.txt
