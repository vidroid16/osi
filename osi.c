#include <stdio.h>
#include <sys/mman.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define A (259 * 1048576)
void* B = (void*) 0x50B9B47D;
#define G 51
#define D 15
//Объявляем переменную которая будет служить указателем
char* ptr;

//Заполнение участка памяти случайными даными в 1 потоке
void* fill(void* data)
{
int thread = *(int*)data;

char* start = A/D * thread + ptr;
char* end = ptr + A - (A/D*thread);
char buffer[G];
int file;
int counter = 0;
for (char* address = start; address < end; address++)
{
	if (counter == G)
	{
		file = open("/dev/urandom", O_RDONLY);
		read(file, buffer, G);
		close(file);
		counter = 0;
	}
	*address = buffer[counter];
	counter++;
	}
}
//Аллокация учатка памяти размером А с адресса В
int allocate(){
	ptr = mmap(B, A, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);	
	if(ptr!= MAP_FAILED){
		return 1;
	}else{
		return 0;
	}
}
//Заполняем весь участок памяти данными в D потоков
void full_fill(){
	pthread_t threads[D];
	for (int i = 0; i < D; i++)
	{
		pthread_create(&threads[i], NULL, fill, &i);

	}
	for (int i = 0; i < D; i++)
	{
		pthread_join(threads[i], NULL);
	}

}
//Деаллокация памяти
int deallocate(){
	ptr = munmap(ptr, A);
	if (ptr != MAP_FAILED){
		return 1;
	}else{
		return 0;
	}
}

int main()
{
	int test;
	printf("Memory not allocated. Press any button to countinue\n");
	scanf("%d",&test);
	allocate();
	printf("Memory allocated. Press any button to countinue\n");
	scanf("%d",&test);
	full_fill();
	printf("Memory was filled by random numbers. Press any button to countinue\n");
	scanf("%d",&test);
	deallocate();
	printf("Memory was deallocated. Press any button to countinue\n");
	scanf("%d",&test);

return 0;
}