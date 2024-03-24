#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h> // For sleep and usleep functions

#define NUMBER_OF_CUSTOMERS 5
#define NUMBER_OF_RESOURCES 3

// Data structures for resource management
int available[NUMBER_OF_RESOURCES]; // Available amount of each resource
int maximum[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES]; // Maximum demand of each customer
int allocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES]; // Amount currently allocated to each customer
int need[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES]; // Remaining need of each customer

// Synchronization primitives
pthread_mutex_t mutex;
sem_t semaphore;

// Function prototypes
int request_resources(int customer_num, int request[]);
int release_resources(int customer_num, int release[]);
void *customer_routine(void *arg);
int check_safety();

int main(int argc, char *argv[]) {
    if (argc != NUMBER_OF_RESOURCES + 1) {
        printf("Usage: %s <Resource 1> <Resource 2> <Resource 3>\n", argv[0]);
        return -1;
    }

    // Initialize available resources
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i] = atoi(argv[i + 1]);
    }

    // Initialize the mutex and semaphore
    pthread_mutex_init(&mutex, NULL);
    sem_init(&semaphore, 0, 1); // Initialized with a value of 1 for binary semaphore usage

    // Sample initialization for maximum, allocation, and need matrices
    // Let's assume each customer can demand up to a maximum of each resource type
    // Allocation starts at 0 for simplicity
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            maximum[i][j] = rand() % (available[j] + 1); // Random maximum demand
            allocation[i][j] = 0; // Start with no allocation
            need[i][j] = maximum[i][j] - allocation[i][j]; // Initial need is maximum demand
        }
    }

    // Create customer threads
    pthread_t threads[NUMBER_OF_CUSTOMERS];
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&threads[i], NULL, customer_routine, (void *)id);
    }

    // Wait for all threads to finish
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Clean up
    pthread_mutex_destroy(&mutex);
    sem_destroy(&semaphore);

    return 0;
}


int request_resources(int customer_num, int request[]) {
    pthread_mutex_lock(&mutex);
    // Check if request is less than or equal to need
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        if (request[i] > need[customer_num][i]) {
            pthread_mutex_unlock(&mutex);
            return -1; // Request exceeds customer's maximum claim
        }
    }

    // Check if request is less than or equal to available
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        if (request[i] > available[i]) {
            pthread_mutex_unlock(&mutex);
            return -1; // Not enough resources available
        }
    }

    // Pretend to allocate requested resources and check system state
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i] -= request[i];
        allocation[customer_num][i] += request[i];
        need[customer_num][i] -= request[i];
    }

    if (check_safety()) { // If allocation leaves system in a safe state
        pthread_mutex_unlock(&mutex);
        return 0; // Request granted
    } else { // System would be in an unsafe state, rollback
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
            available[i] += request[i];
            allocation[customer_num][i] -= request[i];
            need[customer_num][i] += request[i];
        }
        pthread_mutex_unlock(&mutex);
        return -1; // Request denied
    }
}


int release_resources(int customer_num, int release[]) {
    pthread_mutex_lock(&mutex);
    // Release resources and update available, allocation, and need
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i] += release[i];
        allocation[customer_num][i] -= release[i];
        need[customer_num][i] += release[i];
    }
    pthread_mutex_unlock(&mutex);
    return 0;
}


void *customer_routine(void *arg) {
    int customer_num = *(int *)arg;

    // Example: Simulate customer's resource request
    int request[NUMBER_OF_RESOURCES]; // Define an array to hold the request
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        // Request up to the customer's maximum need
        request[i] = need[customer_num][i] > 0 ? rand() % (need[customer_num][i] + 1) : 0;
    }

    if (request_resources(customer_num, request) == 0) {
        printf("Customer %d's request was granted.\n", customer_num);
        // Simulate the customer using the resources
        sleep(1 + rand() % 5); // Random delay to simulate work being done

        // Release the resources after use
        int release[NUMBER_OF_RESOURCES];
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
            release[i] = allocation[customer_num][i]; // Release all allocated resources
        }

        release_resources(customer_num, release);
        printf("Customer %d released resources.\n", customer_num);
    } else {
        printf("Customer %d's request was denied.\n", customer_num);
    }

    pthread_exit(NULL);
}


int check_safety() {
    int work[NUMBER_OF_RESOURCES];
    int finish[NUMBER_OF_CUSTOMERS] = {0};
    
    // Initialize work = available
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        work[i] = available[i];
    }

    int found = 1;
    while (found) {
        found = 0;
        for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
            if (!finish[i]) {
                int j;
                for (j = 0; j < NUMBER_OF_RESOURCES; j++)
                    if (need[i][j] > work[j])
                        break;
                if (j == NUMBER_OF_RESOURCES) { // If all needs can be satisfied
                    for (int k = 0; k < NUMBER_OF_RESOURCES; k++)
                        work[k] += allocation[i][k];
                    finish[i] = 1;
                    found = 1;
                }
            }
        }
    }

    // If all processes can finish, system is in a safe state
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        if (!finish[i])
            return 0; // System is not in a safe state
    }
    return 1; // System is in a safe state
}

