all:
	gcc Server/Broker.c -o Broker -lsqlite3
	gcc Clients/Publisher.c -o Publisher
	gcc Clients/Subscriber.c -o Subscriber
clean:
	rm -f *~ Broker Publisher Subscriber