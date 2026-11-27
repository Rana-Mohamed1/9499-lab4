all: producer consumer
#-pthread for semaphores , -lrt real time library shm_open()
producer: producer.cpp
	g++ -o producer producer.cpp -lrt -pthread  
consumer: consumer.cpp
	g++ -o consumer consumer.cpp -lrt -pthread 
	
clean:
	rm -f producer consumer	
	rm -f /dev/shm/shared_buffer
	rm -f /dev/shm/sem.producerConsumer_s
	rm -f /dev/shm/sem.producerConsumer_n
	rm -f /dev/shm/sem.producerConsumer_e