#include <client.h>
#include <api.h>

#include <unistd.h>

int main(void){
	struct timespec tempo;
	tempo.tv_nsec = 0;
	tempo.tv_sec = 10;
	openConnection("./servWork/daddu", 10, tempo);
	sleep(1);
}