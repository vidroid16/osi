#include <stdio.h>
#include <sys/mman.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <semaphore.h>

//A=259;B=0x50B9B47D;C=mmap;D=15;E=179;F=nocache;G=51;H=seq;I=81;J=avg;K=sema

#define A (259 * 1048576)
void* B = (void*) 0x50B9B47D;
#define D 15
#define E (179 * 1000000)
#define G 51
#define I 81

//Объявляем переменную которая будет служить указателем
char *ptr;
sem_t semaphore;

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

void *writefile(void *data)
{
    int file = *(int *)data;
    char names;
    char *start = ptr;
    char *end = ptr+A;
    char *address = start;
    int current_file;
    int i = 0;
    while(1){
        sem_wait(&semaphore);

        //Выбираю файл для записи в зависимости от потока 
        if(file == 0){
            if ((current_file = open("fil1n1", O_WRONLY | O_CREAT | __O_DIRECT, 0644)) < 0){
                sem_post(&semaphore);
                continue;
            }else{
                printf("Writing to fil1n1\n");
            }
        }else{
            if ((current_file = open("fil1n2", O_WRONLY | O_CREAT | __O_DIRECT, 0644)) < 0){
                sem_post(&semaphore);
                continue;
            }else{
                printf("Writing to fil1n2\n");
            }
        }
        
        //Записываем данные в файл
        for(int k = 0; k< E/512; k++){
            void *buffer_ptr;
            char buffer[512];

            //Создаем буффер из которго будем записывать в файл
            posix_memalign(&buffer_ptr, 512, 512);
            memcpy(buffer_ptr, address, 512);

            if (write(current_file,buffer_ptr, 512)==-1)
            {
                printf("Exeption during writing");
            }
            address += 512;
            if (address + 512 > end)
            {
                address = start;
            }
        }

        //Закрываем файл и освобождаем семафор для другого потока
        close(current_file);
        sem_post(&semaphore);
    }
}

void *readfile(void *data){

    int current_thread = *(int*)data;
    //Сколько байт читается одним потоком
    int thread_area = 4419753;
    int file;
    int file_pointer;
    char buffer[G];
    int mybytes = 0;
    int avg = 0;
    int counter = 0;
    float result = 0.0;
    while(1){
        sem_wait(&semaphore);

        //Т.к файла 2 то пусь первые 41 поток читают из 1го файла, а оставшиеся из 2го
        if(current_thread<41){
            if ((file = open("fil1n1", O_RDONLY)) < 0)
            {
                sem_post(&semaphore);
                continue;
            }
            else
            {
                printf("File1 was opened for reading in tread %d\n",current_thread);
            }
        }else{
            current_thread = current_thread-41;
            if ((file = open("fil1n2", O_RDONLY)) < 0)
            {
                sem_post(&semaphore);
                continue;
            }
            else
            {
                printf("File2 was opened for reading in tread %d\n",current_thread+41);
            }
        }

        //Ставим указатель файла на определенное место в зависимости от номера потока 
        lseek(file, current_thread*thread_area, 0);

        //Подсчитываем AVG
        counter = 0;
        for(int i =0; i<thread_area/G; i++){
            if (file_pointer % G == 0)
            {
                mybytes = read(file, buffer, G);
                file_pointer = 0;
                if (mybytes == 0) {break;}
            }
            char current_value = buffer[file_pointer];
            file_pointer++;
            avg+=current_value;    
            counter = counter+1;
        }
        result = avg/counter;
        printf("%.6f\n", result);

        //Закрываем файл и освобождаем семафор для другого потока
        close(file);
        sem_post(&semaphore);
        
    }
}

int main()
{
    //Инициализируем семафор, начальное значение задаем 1,
    // чтобы 2 потока не могли работать одновременно
    sem_init(&semaphore, 0, 1);

    allocate();    
    full_fill();

    //Создаем 2 потока для записи 1 и 2го файла
    pthread_t wthreads[2];
    int fs[2];
    for (int i = 0; i<2; i++){
        pthread_create(&wthreads[i], NULL, writefile, &i);
    }

    //Создаем I потоков для чтения из файлов. Первая половина читает из 1го файла, 2я из 2го
    pthread_t rthreads[I];
    for (int i = 0; i<I; i++){
        pthread_create(&rthreads[i], NULL, readfile,i);
    }
       
    pthread_join(wthreads[0], NULL);
    sem_destroy(&semaphore);
    return 0;
}