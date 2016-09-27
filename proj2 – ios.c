#include <ctype.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ET_OK 0
#define PAR_ERROR 1
#define SYS_ERROR 2

/*-----------------------------------premenne------------------------------*/
//ukazatele na zdielanu pamat
int *cislo_procesu = NULL,  //aktualne celkove cislo procesu
    *nasadnuty_ludia = NULL;    //momentalny pocet ludi vo voziku

//pre navratove funkcie shmget
int shm_cislo_procesu = 0,
    shm_nasadnuty_ludia = 0;

//procesy
pid_t pid_pasazier,
      pid_vozik,
      pid_mainproc;

//semafory
sem_t
    *sem_zapisovanie,  //vylucny pristup k suboru
    *sem_auta,  //zarucuje, ze v aute bude max tolko ludi aka je kapacita
    *sem_load,  //signalizuje, ci sa moze alebo nemoze nastupovat
    *sem_unload,    //  -    ||     -     vystupovat
    *sem_finish,    //urcuje, kedy sa procesy mozu ukoncit
    *sem_full,    //signalizuje, ci je vozik plny
    *sem_start;     //povoluje voziku spustit load. Zariadi to, ze vozik necaka aktivne

//struktura pre parametre
typedef struct parametre
{
    int p; // pocet procesov reprezentujuci pasaziera (pocet pasazierov)
    int c; // kapacita voziku
    int pt; // maximalna hodnota doby (v milisekundach), po ktorej je generovany novy proces pasaziera;
    int rt; // maximalna hodnota doby (v milisekundach) prejazdu trate
} Tparam;

FILE *file = NULL; // ukazatel na subor



/*-----------------------------------deklaracie funkcii (prototypy)------------------------------*/
int nacitaj_parametre(int argc, char *argv[], Tparam *lok_param);
void ukonci_program() ;     //v pripade prerusenia bezpecne ukonci program
void chyba_vstupnych_parametrov(void);    //v pripade akehokolvek problemu s parametrami vypise chybovu hlasku a ukonci program s navratovou hodnotou 1
int alokuj_zdroje(Tparam param);      //alokuje pamet, inicializuje semafory
void car(Tparam param);     //funkcia voziku
void pasazier(Tparam param, int i);    //funkcia pasazierov
void uvolni_zdroje(void);      //funkcia bezpecne uvolni naalokovane zdroje



/*-----------------------------------realizacia funkcii------------------------------*/
void pasazier(Tparam param, int i)
{
    sem_wait(sem_zapisovanie);
    fprintf(file,"%d\t: P %d\t: started\n",*cislo_procesu,i);
    fflush(file);
    (*cislo_procesu)++;
    sem_post(sem_zapisovanie);

    sem_wait(sem_auta);
    sem_wait(sem_load);
    sem_wait(sem_zapisovanie);
    fprintf(file,"%d\t: P %d\t: board\n",*cislo_procesu,i);
    fflush(file);
    (*cislo_procesu)++;
    sem_post(sem_zapisovanie);
    (*nasadnuty_ludia)++;
    if((*nasadnuty_ludia) < param.c)
    {
        sem_wait(sem_zapisovanie);
        fprintf(file,"%d\t: P %d\t: board order %d\n",*cislo_procesu,i,*nasadnuty_ludia);
        fflush(file);
        (*cislo_procesu)++;
        sem_post(sem_zapisovanie);
        sem_post(sem_load);
    }
    if((*nasadnuty_ludia) == param.c)
    {
        sem_wait(sem_zapisovanie);
        fprintf(file,"%d\t: P %d\t: board last\n",*cislo_procesu,i);
        fflush(file);
        (*cislo_procesu)++;
        sem_post(sem_zapisovanie);
        sem_post(sem_load); //mozem to spravit, pretoze mam aj semafor sem_car s kapacitou auta
        sem_post(sem_full);
    }

    sem_wait(sem_unload);
    sem_wait(sem_zapisovanie);
    fprintf(file,"%d\t: P %d\t: unboard\n",*cislo_procesu,i);
    fflush(file);
    (*cislo_procesu)++;
    sem_post(sem_zapisovanie);
    sem_post(sem_auta);
    (*nasadnuty_ludia)--;
    if((*nasadnuty_ludia) > 0)
    {
        sem_wait(sem_zapisovanie);
        fprintf(file,"%d\t: P %d\t: unboard order %d\n",*cislo_procesu,i,-(*nasadnuty_ludia-param.c));
        fflush(file);
        (*cislo_procesu)++;
        sem_post(sem_zapisovanie);
        sem_post(sem_unload);
    }
    if((*nasadnuty_ludia) == 0)
    {
        sem_wait(sem_zapisovanie);
        fprintf(file,"%d\t: P %d\t: unboard last\n",*cislo_procesu,i);
        fflush(file);
        (*cislo_procesu)++;
        sem_post(sem_zapisovanie);
        sem_post(sem_start);
    }

    sem_wait(sem_finish);
    sem_wait(sem_zapisovanie);
    fprintf(file,"%d\t: P %d\t: finished\n",*cislo_procesu,i);
    fflush(file);
    (*cislo_procesu)++;
    sem_post(sem_zapisovanie);
    sem_post(sem_finish);
    exit(0);
}



void car(Tparam param)
{
    int pocet_jazd = param.p/param.c;   //celkovy a konecny pocet jazd vozika
    int cislo_auta=1;
    int run_time = 0;   //pomocna premenna na generovanie casu prejazdu trate
    int i = 0;

    sem_wait(sem_zapisovanie);
    fprintf(file,"%d\t: C %d\t: started\n",*cislo_procesu,cislo_auta);
    fflush(file);
    (*cislo_procesu)++;
    sem_post(sem_zapisovanie);

    while(i < pocet_jazd)
    {
        sem_wait(sem_start);
        if((*nasadnuty_ludia) == 0)
        {
            sem_wait(sem_zapisovanie);
            fprintf(file,"%d\t: C %d\t: load\n",*cislo_procesu,cislo_auta);
            fflush(file);
            (*cislo_procesu)++;
            sem_post(sem_zapisovanie);
            sem_post(sem_load);
        }
        sem_wait(sem_full);
        if(*nasadnuty_ludia == param.c)
        {
            sem_wait(sem_load);
            sem_wait(sem_zapisovanie);
            fprintf(file,"%d\t: C %d\t: run\n",*cislo_procesu,cislo_auta);
            fflush(file);
            (*cislo_procesu)++;
            sem_post(sem_zapisovanie);
            if(param.rt != 0) {run_time = (random() % (param.rt + 1));}
			usleep(run_time * 1000);
            i++;

            sem_wait(sem_zapisovanie);
            fprintf(file,"%d\t: C %d\t: unload\n",*cislo_procesu,cislo_auta);
            fflush(file);
            (*cislo_procesu)++;
            sem_post(sem_zapisovanie);
            sem_post(sem_unload);
        }
    }
    if(i == pocet_jazd)
    {
        sem_wait(sem_start);
        sem_wait(sem_zapisovanie);
        fprintf(file,"%d\t: C %d\t: finished\n",*cislo_procesu,cislo_auta);
        fflush(file);
        (*cislo_procesu)++;
        sem_post(sem_zapisovanie);
        sem_post(sem_finish);
    }
    exit(0);
}



/*------------------- Parsovanie argumentov ------------------------*/
int nacitaj_parametre(int argc, char *argv[], Tparam *lok_param)
{
    int stav_parametrov = ET_OK;
    // celkom 5 parametrov (4 + nazov programu)
    if(argc == 5)
    {
        // postupne overim jednotlive parametry
        if(isdigit(*argv[1]))
          {lok_param->p=strtol(argv[1], NULL, 10);}
         else
            {stav_parametrov=PAR_ERROR;}

        if(isdigit(*argv[2]))
          {lok_param->c = strtol(argv[2], NULL, 10); }
         else
           {stav_parametrov=PAR_ERROR;}

        if(isdigit(*argv[3]))
          {lok_param->pt=strtol(argv[3], NULL, 10);}
         else
           {stav_parametrov=PAR_ERROR;}

        if(isdigit(*argv[4]))
          {lok_param->rt = strtol(argv[4], NULL, 10);}
         else
           {stav_parametrov=PAR_ERROR;}
        // koniec overovania parametrov
    }
      else
        {stav_parametrov=PAR_ERROR;}

    if(stav_parametrov != ET_OK) {return stav_parametrov;}

    //overim vstupne podmienky pre parametre
    if (lok_param->p <= 0) {chyba_vstupnych_parametrov();}
    if (lok_param->c <= 0) {chyba_vstupnych_parametrov();}
    int pomocne_modulo = lok_param->p % lok_param->c;
    if ((lok_param->c <= 0) || (pomocne_modulo != 0) || (lok_param->c >= lok_param->p)) {chyba_vstupnych_parametrov();}
    if ((lok_param->pt < 0) || (lok_param->pt >= 5001)) {chyba_vstupnych_parametrov();}
    if ((lok_param->rt < 0) || (lok_param->rt >= 5001)) {chyba_vstupnych_parametrov();}

    return stav_parametrov;
}



void chyba_vstupnych_parametrov(void)
{
    fprintf(stderr, "Boli zadane nevhodne parametre, program sa ukonci.\n");
    // uvolnit zdroje nepotrebujem kedze som ziadne dosial nealokoval
    exit(PAR_ERROR);
}



/*-----------------------------------nacitanie zdrojov-------------------*/
int alokuj_zdroje(Tparam param)
{
    int stav_semaforu = ET_OK;
    // sem_zapisovanie: vylucny pristup k suboru
    // sem_auta = semafor auta / zabezpecuje, ze vo voziku nebude viac ludi ako je kapacita
    // sem_load = signalizuje, ci sa da do vozika nastupovat (ano = 1)
    // sem_unliad = spravuje akcciu unload
    // sem_finish = spusti ukoncenie procesov na konci
    // sem_full = ak je vozik plny =1, sluzi na cakanie auta
    // sem_start = podmienka pre zacatie loadingu

    //namapovanie pamate pre semafory
    if((sem_zapisovanie = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED) {stav_semaforu = SYS_ERROR;}
    if((sem_auta = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED) {stav_semaforu = SYS_ERROR;}
    if((sem_load = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED) {stav_semaforu = SYS_ERROR;}
    if((sem_unload = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED) {stav_semaforu = SYS_ERROR;}
    if((sem_finish = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED) {stav_semaforu = SYS_ERROR;}
    if((sem_full = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED) {stav_semaforu = SYS_ERROR;}
    if((sem_start = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED) {stav_semaforu = SYS_ERROR;}

    //inicializacia semaforov
    if(stav_semaforu == ET_OK)
    {
        if(sem_init(sem_zapisovanie, 1, 1) == -1) {stav_semaforu=SYS_ERROR;}
        if(sem_init(sem_auta, 1, param.c) == -1) {stav_semaforu=SYS_ERROR;}
        if(sem_init(sem_load, 1, 0) == -1) {stav_semaforu=SYS_ERROR;}
        if(sem_init(sem_unload, 1, 0) == -1) {stav_semaforu=SYS_ERROR;}
        if(sem_init(sem_finish, 1, 0) == -1) {stav_semaforu=SYS_ERROR;}
        if(sem_init(sem_full, 1, 0) == -1) {stav_semaforu=SYS_ERROR;}
        if(sem_init(sem_start, 1, 1) == -1) {stav_semaforu=SYS_ERROR;}
    }

    //alokovanie pamate pre zdielane premenne
    if(stav_semaforu == ET_OK)
    {
        // shm_cislo_procesu = id pamete pre cislo procesu
        // shm_nasadnuty_ludia = momentalny pocet ludi sediacich vo voziku

        if((shm_cislo_procesu = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666)) == -1) {stav_semaforu=SYS_ERROR;}
        if((shm_nasadnuty_ludia = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666)) == -1) {stav_semaforu=SYS_ERROR;}

        if((cislo_procesu = (int *) shmat(shm_cislo_procesu, NULL, 0)) == NULL) {stav_semaforu=SYS_ERROR;}
        if((nasadnuty_ludia = (int *) shmat(shm_nasadnuty_ludia, NULL, 0)) == NULL) {stav_semaforu=SYS_ERROR;}
    }
    return stav_semaforu;
}



/*-----------------------------------uvolnenie zdrojov-------------------*/
void uvolni_zdroje(void)
{
    fclose(file);

    int uspesnost = ET_OK;
    // uvolnenie semaforov
    if(sem_destroy(sem_zapisovanie) == -1) {uspesnost = SYS_ERROR;}
    if(sem_destroy(sem_auta) == -1) {uspesnost = SYS_ERROR;}
    if(sem_destroy(sem_load) == -1) {uspesnost = SYS_ERROR;}
    if(sem_destroy(sem_unload) == -1) {uspesnost = SYS_ERROR;}
    if(sem_destroy(sem_finish) == -1) {uspesnost = SYS_ERROR;}
    if(sem_destroy(sem_full) == -1) {uspesnost = SYS_ERROR;}
    if(sem_destroy(sem_start) == -1) {uspesnost = SYS_ERROR;}

    //uvolnenie zddielanej pamate
    if(shmctl(shm_cislo_procesu, IPC_RMID, NULL) == -1) {uspesnost = SYS_ERROR;}
    if(shmctl(shm_nasadnuty_ludia, IPC_RMID, NULL) == -1) {uspesnost = SYS_ERROR;}

    if(shmdt(cislo_procesu) == -1) {uspesnost = SYS_ERROR;}
    if(shmdt(nasadnuty_ludia) == -1) {uspesnost = SYS_ERROR;}

    // kontrola chyb pri uvolnovani pamate
    if(uspesnost == SYS_ERROR)
    {
    fprintf(stderr, "Nepodarilo sa spravne uvolnit vsetku pamat alebo uvolnit semafory.\n");
    exit (SYS_ERROR);
    }
}



/*-----------------------------------chybove ukoncenie programu-------------------*/
void ukonci_program()
{
    uvolni_zdroje();
    kill(pid_mainproc, SIGTERM);
    kill(getpid(), SIGTERM);
    exit(SYS_ERROR);
}



/*-----------------------------------main------------------------------*/
int main(int argc, char* argv[])
{
    Tparam param; // struktura pre ulozenie parametrov programu
    int create_time = 0;    //pomocna premenna na generovanie casu

    // parametry ukladam do struktury
    if(nacitaj_parametre(argc, argv, &param) == PAR_ERROR) {chyba_vstupnych_parametrov();}

    pid_t car_pid;
    pid_t potomkovia[param.p];
    pid_t hlavny_proces = 0;

    // handlery pre zachytenie signalu
    signal(SIGTERM, ukonci_program);
    signal(SIGINT, ukonci_program);

    //pre generovanie skutocne nahodnych cisel
    srand(time(NULL) * getpid());

    // otevreme vystupni soubor, v pripade chyby vypis na stdout
    if((file = fopen("proj2.out", "w+")) == NULL)
    {
        fprintf(stderr, "Nepodarilo sa otvorit vystupny subor. Program bude ukonceny.\n");
        // uvolnit zdroje nepotrebujem kedze som ziadne dosial nealokoval
        return EXIT_FAILURE;
    }

    // pre spravny zapis do souboru
	setbuf(file, NULL);

    // alokujem si zdrooje
    int zdroje = alokuj_zdroje(param);
    //kontolujem, ci sa mi ich podarilo alokovat
    if(zdroje != ET_OK)
    {
        if(zdroje == PAR_ERROR) {fprintf(stderr, "Nastala chyba pri vytvarani semaforov. Program bude ukonceny.");}
          else {fprintf(stderr, "Nastala chyba pri alokacii pamati. Program bude ukonceny.");}
        uvolni_zdroje();
        return(EXIT_FAILURE);
    }

    //nastavenie pocitadiel
    *cislo_procesu = 1;
    *nasadnuty_ludia = 0;

    //forkovanie hlavneho procesu
    pid_mainproc = fork();
    if (pid_mainproc < 0)
	{
		fprintf(stderr, "Zlyhalo systemove volanie fork\n");
		ukonci_program();
	}
	else if (pid_mainproc == 0)
        {
        //spustim proces vozika
        pid_vozik = fork();

        if (pid_vozik < 0)
        {
        fprintf(stderr, "Nepodarilo sa vytvorit vozik, zlyhalo systemove volanie fork. Program sa ukonci.");
        ukonci_program();
        }
        else if(pid_vozik == 0) {car(param);} //potomok, teda car(vozik)
        else {car_pid = pid_vozik;}
        waitpid(car_pid, NULL, 0);
        }
     else
      {
      for(int i=1; i <= param.p; i++)  //cyklus pre spustanie procesov pasazierov
       {
        if(param.pt != 0)
        {
        create_time = (random() % (param.rt + 1));  //generovanie "prodlevy" medzi procesmy
        usleep(create_time * 1000);
        }
        //vytvaranie procesov pasazierov
        pid_pasazier = fork();
        if(pid_pasazier < 0)
        {
        fprintf(stderr, "Nepodarilo sa vytvorit pasaziera, zlyhalo systemove volanie fork. Program sa ukonci.");
        ukonci_program();
        }
        else if(pid_pasazier == 0) {pasazier(param,i);}  //potomok, pasazier
        else {potomkovia[i] = pid_pasazier;}
       }
       // cakanie na procesy potomkov
       for(int i = 0; i < param.p; i++)
		{
			waitpid(potomkovia[i], NULL, 0);
		}
      hlavny_proces = pid_mainproc;
      waitpid(hlavny_proces, NULL, 0);
      }

    // po uspesnom ukonceni uvolnim zdroje
    uvolni_zdroje();
    return(ET_OK);
}
