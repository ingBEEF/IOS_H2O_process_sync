#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <fcntl.h>
#include <string.h>

//Makra na mapovani a odmapovani sdilene pameti (prevzato z discordu)
#define MMAP(pointer) {(pointer) = mmap(NULL, sizeof(*(pointer)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);}
#define UNMAP(pointer) {munmap((pointer), sizeof(*(pointer)));} 

//Globalni pocitadla
int *h_count = NULL, *o_count = NULL, *line_count = NULL, *h_molecule = NULL, *molecule_count = NULL, *o_mol_count = NULL;
//Vystupni soubor
FILE *out_file;
//Semafory
sem_t *h_sem = NULL, *o_sem = NULL, *write_sem = NULL, *h_end_sem = NULL, *main_sem = NULL, *h_wait_sem = NULL, *h_molend_sem = NULL, *h_mol_sem = NULL;

//Inicializace vystupniho souboru, sdilenych pameti a semaforu
void init ()
{
    out_file = fopen("proj2.out", "w");
    if (out_file == NULL)
    {
        fprintf(stderr, "Couldnt open output file\n");
        exit(1);
    }
    MMAP(h_count);
    if (h_count == MAP_FAILED)
    {
        fprintf(stderr, "Couldnt map h_count shared memory\n");
        exit(1);
    }
    *h_count = 0;
    MMAP(o_count);
    if (o_count == MAP_FAILED)
    {
        fprintf(stderr, "Couldnt map o_count shared memory\n");
        exit(1);
    }
    *o_count = 0;
    MMAP(line_count);
    if (line_count == MAP_FAILED)
    {
        fprintf(stderr, "Couldnt map write_count shared memory\n");
        exit(1);
    }
    *line_count = 0;
    MMAP(h_molecule);
    if (h_molecule == MAP_FAILED)
    {
        fprintf(stderr, "Couldnt map h_molecule shared memory\n");
        exit(1);
    }
    *h_molecule = 0;
    MMAP(molecule_count);
    if (molecule_count == MAP_FAILED)
    {
        fprintf(stderr, "Couldnt map molecule_count shared memory\n");
        exit(1);
    }
    *molecule_count = 0;
    MMAP(o_mol_count);
    if (o_mol_count == MAP_FAILED)
    {
        fprintf(stderr, "Couldnt map o_mol_count shared memory\n");
        exit(1);
    }
    *o_mol_count = 0;

    if ((h_sem = sem_open("ZYZZ_SEM_H", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
        {
            fprintf(stderr, "h_sem couldnt initialize\n");
            exit(1);
        }
    if ((o_sem = sem_open("ZYZZ_SEM_O", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
        {
            fprintf(stderr, "o_sem couldnt initialize\n");
            exit(1);
        }
    if ((write_sem = sem_open("ZYZZ_SEM_WRITE", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED) 
        {
            fprintf(stderr, "write_sem couldnt initialize\n");
            exit(1);
        }
    if ((h_end_sem = sem_open("ZYZZ_SEM_H_END", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
        {
            fprintf(stderr, "h_sem_end couldnt initialize\n");
            exit(1);
        }
    if ((main_sem = sem_open("ZYZZ_SEM_MAIN", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
        {
            fprintf(stderr, "main_sem couldnt initialize\n");
            exit(1);
        }
    if ((h_wait_sem = sem_open("ZYZZ_SEM_H_WAIT", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
        {
            fprintf(stderr, "h_wait_sem couldnt initialize\n");
            exit(1);
        }
    if ((h_molend_sem = sem_open("ZYZZ_SEM_H_MOLEND_SEM", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
        {
            fprintf(stderr, "h_molend_sem couldnt initialize\n");
            exit(1);
        }
    if ((h_mol_sem = sem_open("ZYZZ_SEM_H_MOL_SEM", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
        {
            fprintf(stderr, "h_mol_sem couldnt initialize\n");
            exit(1);
        }
}

//Uvolneni vystupniho souboru, sdilenych pameti a semaforu
void terminate ()
{
    fclose(out_file);
    UNMAP(h_count);
    UNMAP(o_count);
    UNMAP(line_count);
    UNMAP(h_molecule);
    UNMAP(molecule_count);
    UNMAP(o_mol_count);
    sem_close(h_sem);
    sem_unlink("ZYZZ_SEM_H");
    sem_close(o_sem);
    sem_unlink("ZYZZ_SEM_O");
    sem_close(write_sem);
    sem_unlink("ZYZZ_SEM_WRITE");
    sem_close(h_end_sem);
    sem_unlink("ZYZZ_SEM_H_END");
    sem_close(main_sem);
    sem_unlink("ZYZZ_SEM_MAIN");
    sem_close(h_wait_sem);
    sem_unlink("ZYZZ_SEM_H_WAIT");
    sem_close(h_molend_sem);
    sem_unlink("ZYZZ_SEM_H_MOLEND_SEM");
    sem_close(h_mol_sem);
    sem_unlink("ZYZZ_SEM_H_MOL_SEM");
}


void hydrogen (int nh, int no, int ti)
{
    int wait_time = rand() % (ti + 1) * 1000; //Nastaveni nahodne cekaci doby
    sem_wait(write_sem);
    *line_count += 1;
    *h_count += 1;
    int id = *h_count; //staticka ID vodiku aby nedochazelo ke kolizim v ID
    fprintf(out_file, "%d: H %d: started\n", *line_count, id); //Start vodiku
    fflush(out_file);
    sem_post(write_sem);
    
    usleep(wait_time); //Cekani pred vstupem do fronty

    sem_wait(write_sem);
    *line_count += 1;
    fprintf(out_file, "%d: H %d: going to queue\n", *line_count, id); //Vstup vodiku do fronty
    fflush(out_file);
    sem_post(write_sem);

    sem_wait(h_sem); //Cekani na signal od procesu O

    if ((nh - *h_molecule) < 2 || *o_mol_count == no) //Nastane pri nedostatku atomu
    {
        sem_wait(write_sem);
        *h_molecule += 1;
        *line_count += 1;
        fprintf(out_file, "%d: H %d: not enough O or H\n", *line_count, id); 
        fflush(out_file);
        sem_post(write_sem);
        if (*o_count == no) //Kdyz je H zavolan poslednim procesem O
            sem_post(h_end_sem); //Signal aby O pokracoval
        exit(0);
    }
    sem_wait(write_sem);
    line_count += 1; 
    fprintf(out_file, "%d: H %d: creating molecule %d\n", *line_count, id, *molecule_count); //Pred vytvorenim molekuly
    fflush(out_file);
    sem_post(write_sem);

    sem_post(h_wait_sem); //Signal procesu O o tvorbe molekuly
    sem_wait(h_mol_sem); //Cekani nez se vytvori molekula

    sem_wait(write_sem);
    sem_post(h_molend_sem);
    *h_molecule += 1;
    *line_count += 1;
    fprintf(out_file, "%d: H %d: molecule %d created\n", *line_count, id, *molecule_count); //Molekula vytvrena
    fflush(out_file); 
    sem_post(write_sem);
    exit(0);
}


void oxygen (int nh, int no, int ti, int tb)
{
    int wait_time = rand() % (ti + 1) * 1000;  //Nastaveni nahodne cekaci doby
    int create_time = rand() % (tb + 1) * 1000; //Nastaveni nahodne doby na tvorbu molekuly
    sem_wait(write_sem);
    *line_count += 1;
    *o_count += 1;
    int id = *o_count; //ID jako u hydrogen
    fprintf(out_file, "%d: O %d: started\n", *line_count, id); // Zacatek kysliku
    fflush(out_file);
    sem_post(write_sem);

    usleep(wait_time); //Cekani pred vstupem do fronty

    sem_wait(write_sem);
    *line_count += 1;
    fprintf(out_file, "%d: O %d: going to queue\n", *line_count, id); //Vstup kysliku do fronty
    fflush(out_file);
    sem_post(write_sem);

    sem_wait(o_sem); //Cekani na proces

    if ((nh - *h_molecule) < 2)
    {
        sem_wait(write_sem);
        *line_count += 1;
        fprintf(out_file, "%d: O %d: not enough H\n", *line_count, *o_count);
        fflush(out_file);
        sem_post(write_sem);
        while (*h_molecule != nh) //Dokud zbyvaji procesy H
        {
            sem_post(h_sem); //Pusti proces pres H proces
            sem_wait(h_end_sem); //Ceka na ukonceni H procesu
        }
        if (*o_count == no) //Posledni proces
            sem_post(main_sem); //Uvolneni main procesu (konec)
        sem_post(o_sem); //Pusti dalsi proces
        exit(0);
    }

    sem_wait(write_sem);
    *molecule_count += 1; //Jakoby Vytvoreni molekuly (kvlui funkcionalite)
    sem_post(write_sem);
    sem_post(h_sem);
    sem_post(h_sem);
    sem_wait(write_sem);
    *line_count += 1;
    fprintf(out_file, "%d: O %d: creating molecule %d\n", *line_count, id, *molecule_count);
    fflush(out_file);
    sem_post(write_sem);

    usleep(create_time); //Zacatek vytvoreni molekuly
    sem_wait(h_wait_sem);
    sem_wait(h_wait_sem); //Cekani na oba H
    sem_post(h_mol_sem);
    sem_post(h_mol_sem); //Informace pro H o vytvoreni molekuly

    sem_wait(write_sem);
    *o_mol_count += 1;
    *line_count += 1;
    fprintf(out_file, "%d: O %d: molecule %d created\n", *line_count, id, *molecule_count); //Molekula vytvorena
    fflush(out_file);
    sem_post(write_sem);

    sem_wait(h_molend_sem);
    sem_wait(h_molend_sem); //Cekani na ukonceni H

    if (*o_mol_count == no) //Cisteni procesu
    {
        if (*h_molecule != nh)
        {
            while(*h_molecule != nh)
            {
                sem_post(h_sem);
                sem_wait(h_end_sem);
            }
        }
        sem_post(main_sem);
    }
    sem_post(o_sem); //Spusteni dalsiho O procesu
    exit(0);
}

void create (int no, int nh, int ti, int tb)
{
    pid_t fork_pid;
    int i = 0;
    for (i = 0; i < no; i++)
    {
        fork_pid = fork();
        if(!fork_pid)
            oxygen(nh, no, ti, tb); //Generovani procesu O
    }
    for (i = 0; i < nh; i++)
    {
        fork_pid = fork();
        if(!fork_pid)
            hydrogen(nh, no, ti); //Generovani procesu H
    }
    sem_post(o_sem); //Spusteni procesu tvorby molekuly
}

int main (int argc, char *argv[])
{
    if (argc != 5)
    {
        fprintf(stderr, "Error: Spatny pocet vstupu\n"); //Kontrola vstupu
        return 1;
    }
    srand(time(NULL));
    char* trash = NULL; //Pointer pro kontrolu vstupu
    long int no = strtol(argv[1], &trash, 10);
    if (*trash != '\0' || no <= 0)
    {
        fprintf(stderr, "Error: Nevalidni hodnota NO\n"); //Kontrola vstupu
        return 1;
    }
    long int nh = strtol(argv[2], &trash, 10);
    if (*trash != '\0' || nh < 0)
    {
        fprintf(stderr, "Error: Nevalidni hodnota NH\n"); //Kontrola vstupu
        return 1;
    }

    if(strcmp(argv[3], "") == 0)
    {
        fprintf(stderr,"prazdny ti\n"); //Kontrola vstupu
        return 1;
    }

    if(strcmp(argv[4], "") == 0)
    {
        fprintf(stderr,"prazdny tb\n"); //Kontrola vstupu
        return 1;
    }

    long int ti = strtol(argv[3], &trash, 10);
    if (*trash != '\0' || ti < 0 || ti > 1000)
    {
        fprintf(stderr, "Error: Nevalidni hodnota TI\n"); //Kontrola vstupu
        return 1;
    }
    long int tb = strtol(argv[4], &trash, 10);
    if (*trash != '\0' || tb < 0 || tb > 1000)
    {
        fprintf(stderr, "Error: Nevalidni hodnota TB\n"); //Kontrola vstupu
        return 1;
    }
    init(); //Inicializace
    create(no, nh, ti, tb); //Vytvoreni Fronty s procesy
    sem_wait(main_sem); //Cekani na posledni proces (posledni vytvoreni molekuly)
    terminate(); //Destruktor
}
