# Question 2

- Input taken first.
- Created all threads at the beginning using pthread_create()
- Created a thread for each chef.
    - In the chef thread function:
        - chef sleeps for his arrival time
        - check if a pizza has to be processed using pthread_cond_timedwait()
        - exit if a pizza does not get assigned and the chef has reached his exit_time limit
        - assign the pizza to the chef
        - exit if the preparation time of the pizza will exceed the chef's exit_time
        - start preparing the pizza
        - start gathering ingredients
        - if enough ingredients are not available, exit the function
        - allocate oven for the pizza. If oven is not immediately available, pthread_cond_timedwait() is used,
        - exit the function if oven does not get allocated
        - put the pizza in the oven and start preparing
        - sleep while pizza prepares
        - now, if the pizza was rejected due to lack of time, we put it in the queue again so that some other chef may be able to prepare it
        - if time is still remaining before chef exits, we again go to the start of the loop (where pizza assignment takes place).
        - exit the function

- Created a thread for each customer.
    - sleep till the arrival time of the customer
    - if the drive thru customers exceed the limit, we wait until someone exits (not possible because of the leniency provided in tcs)
    - the customer exits if there are no chefs in the restaurant at the point of arrival
    - else the order is placed and printed on terminal
    - a queue is maintained for the orders
    - again, the customer exits if there are no chefs working
    - otherwise, a signal is broadcast to all the chef threads to start preparing the order
    - the customer conditionally waits till the order prepares
    - check for order status of the customer and print it on the terminal
    - exit the function

- Joined all threads using pthread_join()

## Follow-Up Questions

1. Semaphores may be used. They are intitialed to the primary storage value i.e. the pizzas can be stored. Everytime the chef wants to put the pizza, we use the *sem_trywait()* function. This function will allow entry if value of semaphore is greater than 0, else block until the value becomes positive.

2. Orders in the restaurant can be cancelled midway through processing because of the following reasons:
    1. chef does not have enough time left to prepare the pizza
    2. unavailability of ingredients.\

Following measures can be taken:

    Use the concept of a buffer time, say x along with the actual preparation time of of the pizza. We then use this total time (prep_time + x) to check if we want to reject an order or not. This will increase the frequency of the rejected orders at the very start rather than somewhere in the middle and thus improve the restaurant's ratings. 

3. Note that after the pizza gets assigned to the chef, we check if they have enough ingredients available to prepare the pizza. If they don't, we reject the pizza. However, if we are allowed to replenish our ingredients, we can use cond_timedwait() and wait for a stipulated amount of time to see if the ingredients become available. Note that this time, say x, added with preparation time, p cannot excceed the chefs exit time (x + p <= exit_time).
