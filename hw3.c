#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define MAX_AUTOMOBILES 8
#define MAX_PICKUPS 4
#define MAX_TEMP_SPOTS 1
#define NUM_OWNERS 20
#define NUM_OWNERS_THREAD 2
#define NUM_ATTENDANTS_THREAD 2


typedef struct {// shared memory structures
    int free_automobiles;
    int free_pickups;
    int temp_spots;
    int type_auto[NUM_OWNERS];// array forstore vehicle type (0 for pickup, 1 for automobile) 
} SharedMemory;

SharedMemory entrance = {MAX_AUTOMOBILES, MAX_PICKUPS, MAX_TEMP_SPOTS};

int owners_served = 0;
int parking_full = 0; // flag to parking is full


sem_t newPickup, inChargeforPickup;// semaphores for managing pickups
sem_t newAutomobile, inChargeforAutomobile;// semaphores for managing automobiles
sem_t temp_spots; // semaphore for managing temporary spots
sem_t owners_served_sem; // semaphore for controlling access to the owners_served counter

void *carOwner(void *arg);
void *carAttendant(void *arg);
int isAutomobile();

int main() {
    // initialize semaphores
    sem_init(&newPickup, 0, 0);
    sem_init(&inChargeforPickup, 0, MAX_PICKUPS);
    sem_init(&newAutomobile, 0, 0);
    sem_init(&inChargeforAutomobile, 0, MAX_AUTOMOBILES);
    sem_init(&temp_spots, 0, MAX_TEMP_SPOTS);
    sem_init(&owners_served_sem, 0, 1); 

    pthread_t owner_threads[NUM_OWNERS_THREAD], attendant_threads[NUM_ATTENDANTS_THREAD];// create threads for owners and attendants
    int i;
    int thread_num = 0;

    for (i = 0; i < NUM_OWNERS; i++) {// create car owner threads and attendats threads
        pthread_create(&owner_threads[thread_num], NULL, carOwner, (void *)(long)i);
        pthread_create(&attendant_threads[thread_num], NULL, carAttendant, (void *)(long)i);
        thread_num = (i + 1) % 2;
    }

    for (i = 0; i < NUM_OWNERS_THREAD; i++) {// wait for car owner threads to finish
        pthread_join(owner_threads[i], NULL);
        pthread_detach(owner_threads[i]); // detach the thread
    }

    for (i = 0; i < NUM_ATTENDANTS_THREAD; i++) {// wait for car attendant threads to finish
        pthread_join(attendant_threads[i], NULL);
        pthread_detach(attendant_threads[i]); // detach the thread
    }
   
    parking_full = 1; // signal attendants to exit
    sem_post(&newPickup);
    sem_post(&newAutomobile);

    // Clean up semaphores
    sem_destroy(&newPickup);
    sem_destroy(&inChargeforPickup);
    sem_destroy(&newAutomobile);
    sem_destroy(&inChargeforAutomobile);
    sem_destroy(&temp_spots);
    sem_destroy(&owners_served_sem);

    return 0;
}

void *carOwner(void *arg) {
    int id = (int)(long)arg;
    int isAuto = isAutomobile();
    entrance.type_auto[id] = isAuto;

    sem_wait(&temp_spots);// sait for a temporary spot
    printf("Vehicle %d (%s) arrives and is placed in the temporary parking spot.\n", id, isAuto ? "Automobile" : "Pickup");

    if (isAuto) {// car attendant and wait for parking
        sem_post(&newAutomobile); // signal an automobile attendant
        sem_wait(&inChargeforAutomobile); // wait for the automobile to be parked
    } else {
        sem_post(&newPickup); // signal a pickup attendant
        sem_wait(&inChargeforPickup); // wait for the pickup to be parked
    }

    sem_post(&temp_spots);// car parked so thatrelease the temporary spot

    sem_wait(&owners_served_sem);
    owners_served++;// owners_served counter ++
    sem_post(&owners_served_sem);

    return NULL;
}

void *carAttendant(void *arg) {
    int id = (int)(long)arg;//attendant id
    int isPickup = !(entrance.type_auto[id]);// determine attendant is pickups or automobiles

    if(!parking_full){
        if (isPickup) {
            sem_wait(&newPickup);// wait for an pickup owner to signal
            if (entrance.free_pickups > 0) {
                entrance.free_pickups--;// decrement the number of available pickup spots
                printf("Pickup is moved from temporary to a pickup parking spot. %d pickup spots left.\n", entrance.free_pickups);
                sem_post(&inChargeforPickup);// signal the owner that the pickup is parked
                if (id == NUM_OWNERS-1 ) {
                    parking_full = 1;// set the parking_full flag (not full but there is no more owner)
                    printf("There is no more owner left.\n");
                    return NULL;
                }
            } else {
                printf("No pickup spots available.\n");
                sem_post(&temp_spots); // return the vehicle to the temporary spot
                if (id == (NUM_OWNERS-1 )) {
                    parking_full = 1;// set the parking_full flag (not full but there is no more owner)
                    printf("There is no more owner left.\n");
                    return NULL;
                }
            }
        } else {
            sem_wait(&newAutomobile);// wait for an automobile owner to signal
            if (entrance.free_automobiles > 0) {
                entrance.free_automobiles--;// decrement the number of available automabile spots
                printf("Automobile is moved from temporary to a car parking spot. %d spots left.\n", entrance.free_automobiles);
                sem_post(&inChargeforAutomobile);// signal the owner that the automobile is parked
                if (id == NUM_OWNERS-1 ) {
                    parking_full = 1;// set the parking_full flag (not full but there is no more owner)
                    printf("There is no more owner left.\n");
                    return NULL;
                }
            } else {
                printf("No automobile spots available.\n");
                sem_post(&temp_spots); // return the vehicle to the temporary spot
                if (id == NUM_OWNERS-1 ) {
                    parking_full = 1;// set the parking_full flag (not full but there is no more owner)
                    printf("There is no more owner left.\n");
                    return NULL;
                }
            }
        }
  
        if (entrance.free_automobiles == 0 && entrance.free_pickups == 0) {// check if all parking spots are full
            parking_full = 1;
            printf("All parking spots are full. Exiting program.\n");// exit the program all parking spots are full
            exit(1);
        }
    }
    return NULL;
}

int isAutomobile() {
    static int initialized = 0;
    if (!initialized) {
        srand(time(NULL));
        initialized = 1;
    }
    return rand() % 2; // 1 for automobile, 0 for pickup
}