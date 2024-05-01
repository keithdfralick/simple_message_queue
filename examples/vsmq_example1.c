/*
** This is free and unencumbered software released into the public domain.
**
** Refer to LICENSE for additional information.
*/

/*
** This example reads/outputs /usr/share/dict/words. You may need to 
** install a package for this to work correctly.
**
** Compile: gcc smq.c vsmq.c vsmq_example1.c -pthread -o vsmq_example1
*/
#include "vsmq.h"

#define FILENAME    "/usr/share/dict/words"
#define READ_THREADS    1

static vSMQ queue;

void *dict_readers(void *args) {
    char *word;
    int sz;

    /* Just wait 1 sec */
    sleep (1);

    /* read the word, wait up to 1 second */
    while ((word = vsmq_recv(queue, &sz, 1000))) {
        printf("%s [length = %d]\n", word, sz);

        /* must free memory here */
        free (word);
    }

    return NULL;
}

int main(void) {
    FILE *fp;
    char buffer[1024];
    pthread_t tids[READ_THREADS];
    int i;

    /* open the file */
    if (!(fp = fopen(FILENAME, "r"))) {
        fprintf(stderr, "No \"%s\" found :-(\n", FILENAME);
        return -1;
    }

    /* attempt to open queue */
    if (!(queue = vsmq_create(10))) {
        fprintf(stderr, "Unable to open vSMQ\n");
        fclose (fp);
        return -1;
    }

    /* start threads */
    for (i = 0; i < READ_THREADS; i++)
        pthread_create(&tids[i], NULL, dict_readers, NULL);

    /* read each word */
    while (fgets(buffer, sizeof(buffer), fp)) {
        char *p;
        int sz;

        /* strip carriage return and new line */
        if ((p = strchr(buffer, '\r')))
            *p = '\0';
        if ((p = strchr(buffer, '\n')))
            *p = '\0';

        /* ignore blank lines */
        if (!*buffer)
            continue;

        /* get the length of the string */
        sz = strlen(buffer);

        vsmq_send(queue, buffer, sz, 1500);
    }

    /* close the file */
    fclose (fp);

    /* join to wait for threads */
    for (i = 0; i < READ_THREADS; i++)
        pthread_join(tids[i], NULL);

    return 0;
}