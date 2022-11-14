# Question 1

- Created a thread for each student
- sort according to arrival time and then IDs for FCFS
- maintain all variables (disappointed students, time wasted, etc)
- sleep (progressively more in nanoseconds) to implement fcfs
- student then arrives and tries to get hold of a machine
- if machines available are 0, we to pthread_cond_timedwait() and wait till a machine becomes available (wait only till the student has patience)
- if machine was available / became available, start washing after locking on the machine using sem_trywait()
- sleep for wash time
- release the lock machine (sempahore)
- do cond_signal() to signal that a machine has become available
- leave the function
- if the student could not get hold of the machine, add 1 to the disappointed varibale and add to time_wasted and leave.